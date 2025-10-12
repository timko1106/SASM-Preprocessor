#include "common.h"

static state_t shorted = {};

typedef struct {
	word m_cnt, l_cnt, s_cnt, p_cnt, j_cnt, n_cnt;
} local_state;

static void shortify (const char** _p) {
	const char* p = *_p;

	while (*p != LF && *p) {
		char c = *p;
		if (c == COMMENT) {
			break;
		}
		if (!isspace (c)) {
			shorted.buffer[shorted.length++] = c;
		}
		++p;
	}
	while (*p != LF && *p) {
		shorted.buffer[shorted.length++] = *p++;
	}
	shorted.buffer[shorted.length] = 0;
	if (*p == LF) ++p;

	*_p = p;
}

typedef enum : uint8_t {
	R0 = 0,
	R1 = 1,
	R2 = 2,
	R3 = 3,
	RC = 4,
	RF = 5
} reg_t;

//Entity: register number/address/number/mark.
typedef enum : uint8_t {
	E_REGNO,
	E_ADDR,
	E_NUMBER,
	E_MARK,
} ent_t;

typedef struct {
	ent_t _type;
	union {
		reg_t regno;
		word address;
		word number;
		word mark_id;
	};
} entity_t;

//regno - not NULL
static bool check_common_regno (const char* p, const char** next_p, reg_t* regno) {
	if (p[0] == '0' && '0' <= p[1] && p[1] < '0' + REGISTERS) {
		*next_p = p + 2;
		*regno = p[1] - '0';
		return true;
	}
	if ('0' <= p[0] && p[0] < '0' + REGISTERS) {
		*next_p = p + 1;
		*regno = p[0] - '0';
		return true;
	}
	return false;
}

static bool check_common_regno_2 (const char* p, const char** next_p, reg_t* regno) {
	if (!(p[0] == R_START || p[0] == r_START)) {
		return false;
	}
	return check_common_regno (p + 1, next_p, regno);
}

static bool maybe_register (const char* p, const char** next_p, reg_t* regno) {
	if (!(p[0] == R_START || p[0] == r_START)) {
		return false;
	}
	if (check_common_regno (p + 1, next_p, regno)) {
		return true;
	}
	if (p[1] == REG_JMP) {
		*next_p = p + 2;
		*regno = RC;
		return true;
	}
	if (p[1] == REG_CMP) {
		*next_p = p + 2;
		*regno = RF;
		return true;
	}
	return false;
}

static bool is_hex (char c) {
	return isdigit (c) || ('a' <= c && c <= 'f')
			|| ('A' <= c && c <= 'F');
}
static word from_hex (char c) {
	return (isdigit (c) ? c - '0' : 
		10 + ( ('a' <= c && c <= 'f') ? 
			c - 'a' :
			c - 'A'
		)
		);
}

static int parse_number (const char* p, const char** next_p, word* dst) {
	bool is_neg = false;
	if (*p == '-') {
		is_neg = true;
		++p;
	}
	if (p[0] == '0' && p[1] == 'x') {
		p += 2;
		if (!is_hex (*p)) {
			PANIC ("At least 1 hex digit needed:\n\"%s\"\n", p - 2 - is_neg);
			return EXIT_FAILURE;
		}
		word value = from_hex (*p++);
		if (is_hex (*p)) {
			value <<= 4;
			value += from_hex (*p++);
		}

		if (is_neg) {
			value = -value;
		}
		*dst = value;
		*next_p = p;
		return EXIT_SUCCESS;
	}
	//Consider decimal.
	if (!isdigit (*p)) {
		PANIC ("At least 1 decimal digit needed:\n\"%s\"\n", p - is_neg);
		return EXIT_FAILURE;
	}
	word value = *p - '0';
	++p;
	if (isdigit (*p)) {
		value *= 10;
		value += (*p++) - '0';
		if (isdigit (*p)) {
			unsigned tmp = value;
			tmp *= 10;
			tmp += (*p++) - '0';
			if (tmp > 0xFF) {
				PANIC ("Number overflow (%u)\n\"%s\"\n", tmp, p - 3);
				return EXIT_FAILURE;
			}
			value = tmp;
		}
	}
	if (is_neg) {
		value = -value;
	}

	*dst = value;
	*next_p = p;
	return EXIT_SUCCESS;
}

static int parse_entity (const char* p, const char** next_p, entity_t* ent, parsed_t* prog, bool extended_mode, local_state* st) {
	reg_t regno;
	if (maybe_register (p, next_p, &regno)) {
		ent->_type = E_REGNO;
		ent->regno = regno;
		return EXIT_SUCCESS;
	}
	if (*p != MARK) {
		ent->_type = E_NUMBER;
		return parse_number (p, next_p, &ent->number);
	}
	++p;
	if (isdigit (*p) || *p == '-') {
		//It's address.
		ent->_type = E_ADDR;
		return parse_number (p, next_p, &ent->address);
	}
	if (is_valid (*p)) {
		//It's mark.
		ent->_type = E_MARK;
		const char* const name = p;
		do {
			++p;
		} while (is_valid (*p));
		const unsigned mark_len = p - name;
		word m_id = 0;
		if (mark_len == 1 && (*name == ADDR || *name == NEXT)) {
			if (!extended_mode) {
				PANIC ("Extended marks are not supported!\n\"%s\"\n", p - 1);
				return EXIT_FAILURE;
			}
			if (st->m_cnt >= prog->m_count) {
				PANIC ("Too many marks\n\"%s\"\n", p - 1);
				return EXIT_FAILURE;
			}
			parsed_ext_t* _prog = (parsed_ext_t*) prog;
			const bool is_p = *name == ADDR;
			word* mark = (is_p ? (_prog->p_marks + st->p_cnt) : (_prog->n_marks + st->n_cnt));
			if (*mark) {
				PANIC ("%c_mark declared twice\n\"%s\"\n", is_p ? 'p' : 'n', p - 1);
				return EXIT_FAILURE;
			}
			if (st->m_cnt == 0) {
				PANIC ("%c_mark shouldn't be mark #1\n\"%s\"\n", is_p ? 'p' : 'n', p - 1);
				return EXIT_FAILURE;
			}
			m_id = st->m_cnt;

			mark_info_t* info = prog->marks + m_id;
			info->name = name;
			info->mark_len = 1;
			info->cmd_id = prog->cmd_count;
			
			*mark = m_id;
			++st->m_cnt;
		} else if (mark_len == 1 && (*name == LOAD || *name == STORE || *name == JUMP)) {
			PANIC ("Used special mark (that shouldn't be used at all!)\n\"%s\")", p - 1);
			return EXIT_FAILURE;
		} else {
			m_id = st->m_cnt;
			for (word i = 0; i < st->m_cnt; ++i) {
				if (prog->marks[i].mark_len == mark_len && memcmp (name, prog->marks[i].name, mark_len) == 0) {
					m_id = i;
					break;
				}
			}
			if (m_id == st->m_cnt) {
				if (m_id >= prog->m_count) {
					PANIC ("Too many marks\n\"%s\"\n", p - 1);
					return EXIT_FAILURE;
				}
				mark_info_t* info = prog->marks + m_id;
				info->name = name;
				info->mark_len = mark_len;
				info->cmd_id = prog->cmd_count;
				++st->m_cnt;
			}
		}
		ent->mark_id = m_id;
		*next_p = p;
		return EXIT_SUCCESS;
	}
	PANIC ("Unknown entity:\n\"%s\"\n", p - 1);
	return EXIT_FAILURE;
}

//Проверка на правильное определение mark (если оно вообще есть).
static int check_start_mark (parsed_t *prog, bool extended_mode, local_state *st, unsigned _id, const char** _loc) {
	const char* loc = *_loc;
	if (*loc == MARK_DEF) {
		PANIC ("Empty mark\n\"%s\"\n", loc);
		return EXIT_FAILURE;
	}
	const char* ptr = NULL;
	reg_t _reg;
	if (!isdigit (loc[0]) && is_valid (loc[0]) && (!maybe_register (loc, &ptr, &_reg) || is_valid (*ptr) || *ptr == MARK_DEF)) {
		//It should be mark name + ':' (MARK_DEF)
		if ((loc[0] == LOAD || loc[0] == STORE) && loc[1] == MARK_DEF) {
			PANIC ("Illegal usage of parametric mark\n\"%s\"\n", loc);
			return EXIT_FAILURE;
		}
		if ((loc[0] == JUMP || loc[0] == ADDR || loc[0] == NEXT) && loc[1] == ARG_OPEN) {
			PANIC ("Illegal usage of non-parametric mark\n\"%s\"\n", loc);
			return EXIT_FAILURE;
		}
		const word m_id = st->m_cnt;
		mark_info_t* info = prog->marks + m_id;
		//All manipulations inside of IF!
		//st->m_cnt should be incremented INSIDE if!

		if ((loc[0] == LOAD || loc[0] == STORE) && loc[1] == ARG_OPEN) {
			const char* ptr2 = NULL;
			if (!extended_mode) {
				PANIC ("Parametric mark used not in extended mode\n\"%s\"\n", loc);
				return EXIT_FAILURE;
			}
			if (!check_common_regno (loc + 2, &ptr2, &_reg)) {
				PANIC ("Invalid register number\n\"%s\"\n", loc);
				return EXIT_FAILURE;
			}
			if (*ptr2 != ARG_CLOSE) {
				PANIC ("Invalid mark closing\n\"%s\"\n", loc);
				return EXIT_FAILURE;
			}
			if (m_id >= prog->m_count) {
				PANIC ("Too many marks:\n\"%s\"\n", loc);
				return EXIT_FAILURE;
			}
			parsed_ext_t* _prog = (parsed_ext_t*) prog;
			const bool is_store = loc[0] == STORE;
			
			info->name = loc;
			info->mark_len = 1;
			mark_param_t* param = (is_store ? _prog->s_marks + st->s_cnt : _prog->l_marks + st->l_cnt);
			param->mark_id = m_id;
			param->reg = _reg;

			++st->m_cnt;
			if (is_store) {
				++st->s_cnt;
			} else {
				++st->l_cnt;
			}
			loc = ptr2 + 1;//ptr2+1 - :
			++loc;//ptr2 + 2 - next
		} else if ((loc[0] == ADDR || loc[0] == NEXT) && loc[1] == MARK_DEF) {
			if (!extended_mode) {
				PANIC ("Reference mark used not in extended mode\n\"%s\"\n", loc);
				return EXIT_FAILURE;
			}
			parsed_ext_t* _prog = (parsed_ext_t*) prog;
			//Address mark should be used BEFORE!
			const word mark_id = (loc[0] == ADDR ? _prog->p_marks[st->p_cnt] : _prog->n_marks[st->n_cnt]);

			if (prog->marks[mark_id].mark_len != 1 || prog->marks[mark_id].name[0] != loc[0]) {
				PANIC ("Reference mark defined before actual usage\n\"%s\"\n", loc);
				return EXIT_FAILURE;
			}
			if (prog->marks[mark_id].cmd_id != prog->cmd_count) {
				PANIC ("Reference mark is broken!\n\"%s\"\n", loc);
				return EXIT_FAILURE;
			}
			info = prog->marks + mark_id;
			info->name = loc;
			info->mark_len = 1;

			if (loc[0] == ADDR) {
				++st->p_cnt;
			} else {
				++st->n_cnt;
			}
			loc = loc + 2;
		} else if (loc[0] == JUMP && loc[1] == MARK_DEF) {
			if (!extended_mode) {
				PANIC ("Jump mark used not in extended mode\n\"%s\"\n", loc);
				return EXIT_FAILURE;
			}
			if (m_id >= prog->m_count) {
				PANIC ("Too many marks:\n\"%s\"\n", loc);
				return EXIT_FAILURE;
			}
			parsed_ext_t* _prog = (parsed_ext_t*) prog;

			info->mark_len = 1;
			info->name = loc;
			_prog->j_marks[st->j_cnt] = m_id;

			++st->j_cnt;
			++st->m_cnt;


			loc = loc + 2;
		} else {
			//Просто mark... искренне надеемся.
			const char* const name = loc;
			unsigned name_len = 0;
			while (is_valid (name[name_len])) {
				++name_len;
			}
			if (name[name_len] != MARK_DEF) {
				PANIC ("Invalid mark definition!\n\"%s\"\n", loc);
				return EXIT_FAILURE;
			}
			word mark_id = st->m_cnt;
			bool found_any = false;
			for (word i = 0; i < st->m_cnt; ++i) {
				if (prog->marks[i].mark_len == name_len && memcmp (prog->marks[i].name, name, name_len) == 0) {
					found_any = true;
					mark_id = i;
					break;
				}
			}
			if (!found_any && mark_id >= prog->m_count) {
				PANIC ("too many marks:\n\"%s\"\n", loc);
				return EXIT_FAILURE;
			}
			if (found_any && prog->marks[mark_id].cmd_id != prog->cmd_count) {
				PANIC ("mark already defined!\n\"%s\"\n", loc);
				return EXIT_FAILURE;
			}

			info = prog->marks + mark_id;
			info->name = name;
			info->mark_len = name_len;
			if (!found_any) {
				++st->m_cnt;
			}

			loc = loc + name_len + 1;
		}
		info->cmd_id = _id;
		//prog->commands[_id].mark_id = (word)(info - prog->marks);

	}
	prog->commands[_id].mark_id = prog->m_count;
	*_loc = loc;
	return EXIT_SUCCESS;
}

static int check (parsed_t* prog, bool extended_mode, local_state *st, unsigned _id, const char* start, unsigned length) {
	command_ext_t* cmd = prog->commands + _id;
	cmd->mark_id = prog->m_count;
	cmd->_.src = cmd->_.dst = 0;
	cmd->_.param = 0;

	if (length == 0 || (length == 1 && start[0] == ';')) {
		cmd->comment = "";
		cmd->comment_length = 0;
		cmd->_._type = C_NONE;
		return EXIT_SUCCESS;
	}
	if (start[0] == ';') {
		cmd->comment = start + 1;
		cmd->comment_length = length;
		cmd->_._type = C_NONE;
		return EXIT_SUCCESS;
	}
	//Shortest: R0<-0
	if (length < 5) {
		PANIC ("Not enough length:\n\"%s\"\n", start);
		return EXIT_FAILURE;
	}
	const char* loc = start;
	if (check_start_mark (prog, extended_mode, st, _id, &loc) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	entity_t ent = {};
	if (parse_entity (loc, &loc, &ent, prog, extended_mode, st) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}
	if (loc[0] != SEND_1 || loc[1] != SEND_2) {
		PANIC ("after 1-st operand should be \"<-\"\n\"%s\"\n", start);
		return EXIT_FAILURE;
	}
	loc += 2; //<-
	
	switch (ent._type) {
	case E_REGNO:;
	{
		const reg_t regno = ent.regno;
		if (regno == RF) {
			reg_t dst = {}, src = {};
			if (!check_common_regno_2 (loc, &loc, &dst)) {
				PANIC ("2-nd operand of CMP should be common register!\n\"%s\"\n", start);
				return EXIT_FAILURE;
			}
			if (*loc != CMP_SIGN) {
				PANIC ("comparation should be like RF <- R0 ~ R1\n\"%s\"\n", start);
				return EXIT_FAILURE;
			}
			++loc;
			if (!check_common_regno_2 (loc, &loc, &src)) {
				PANIC ("3-rd operand of CMP should be common register!\n\"%s\"\n", start);
				return EXIT_FAILURE;
			}
			cmd->_.src = src;
			cmd->_.dst = dst;
			cmd->_._type = C_CMP;
			break;
		}//1
		if (regno != RC && loc[0] == RAND) {
			cmd->_.dst = regno;
			cmd->_._type = C_RAND;
			++loc;
			break;
		}//2
		//2nd operand.
		entity_t ent2 = {};
		if (*loc == REF_OPEN && regno != RC && regno != RF) {
			++loc;
			if (parse_entity (loc, &loc, &ent2, prog, extended_mode, st) == EXIT_FAILURE) {
				PANIC ("Trace: \"%s\"\n", loc);
				return EXIT_FAILURE;
			}
			if (*loc != REF_CLOSE) {
				PANIC("After reference should be %c!\n\"%s\"\n", REF_CLOSE, loc);
				return EXIT_FAILURE;
			}
			++loc;
			cmd->_.dst = regno;
			cmd->_._type = C_MOV_V;
			if (ent2._type == E_ADDR) {
				cmd->_.param = ent2.address;
			} else if (ent2._type == E_MARK) {
				cmd->mark_id = ent2.mark_id;
			} else {
				PANIC("Invalid reference argument!\"%s\"\n", start);
				return EXIT_FAILURE;
			}
			break;
		}

		if (parse_entity (loc, &loc, &ent2, prog, extended_mode, st) == EXIT_FAILURE) {
			PRINT("Trace: \"%s\"\n", loc);
			return EXIT_FAILURE;
		}
		if (regno == RC) {
			if (ent2._type != E_ADDR && ent2._type != E_MARK) {
				PANIC ("2-nd operand of JMP should be address or mark\n\"%s\"\n", start);
				return EXIT_FAILURE;
			}
			if (ent2._type == E_ADDR) {
				cmd->_.param = ent2.address;
			} else {
				cmd->mark_id = ent2.mark_id;
			}
			cmd->_._type = C_JMP;//3
			if (*loc == ARG_OPEN) {
				++loc;

				if (*loc == J_CARRY) {
					cmd->_._type = C_JMP_C;//4
				} else if (*loc == J_SIGN) {
					cmd->_._type = C_JMP_S;//5
				} else if (*loc == J_ZERO) {
					cmd->_._type = C_JMP_Z;//6
				} else {
					PANIC ("Unknown jump condition:\n\"%s\"\n", start);
					return EXIT_FAILURE;
				}
				++loc;
				if (*loc != ARG_CLOSE) {
					PANIC ("Wrong jump format:\n\"%s\"\n", start);
					return EXIT_FAILURE;
				}
				++loc;
			}
			break;
		}
		cmd->_.dst = regno;
		if (ent2._type == E_NUMBER) {
			cmd->_.param = ent2.number;
			cmd->_._type = C_MOV_V;
			break;
		}//7
		if (ent2._type == E_ADDR || ent2._type == E_MARK) {
			if (ent2._type == E_ADDR) {
				cmd->_.param = ent2.address;
			} else {
				cmd->mark_id = ent2.mark_id;
			}
			cmd->_._type = C_LOAD;
			break;
		}//8
		if (ent2.regno == RC || ent2.regno == RF) {
			PANIC ("2nd argument shouldn't be RC or RF!\n\"%s\"\n", start);
			return EXIT_FAILURE;
		}
		//E_REGNO
		cmd->_.src = ent2.regno;
		if (*loc == 0 || *loc == LF || *loc == COMMENT) {
			cmd->_._type = C_MOV;
			break;
		}//9
		if (loc[0] == '+' && loc[1] == '+') {
			cmd->_._type = C_INC;
			loc += 2;
			break;
		}//10
		if (ent2.regno != regno) {
			PANIC ("for arithmetic operations second and first arguments should be equal\n\"%s\"\n", start);
			return EXIT_FAILURE;
		}
		const char op = *loc;
		cmd->_._type = (op == '+' ? C_ADD : 
					(op == '-' ? C_SUB :
						(op == '*' ? C_MUL :
							(op == '/' ? C_DIV : C_NONE)
						)
					)
				);
		if (cmd->_._type == C_NONE) {
			PANIC ("Unknown arithmetic operator %c\n\"%s\"\n", op, start);
			return EXIT_FAILURE;
		}
		++loc;
		
		reg_t src = {};
		if (!check_common_regno_2 (loc, &loc, &src)) {
			PANIC ("3-rd argument should be valid common register\n\"%s\"\n", start);
			return EXIT_FAILURE;
		}
		cmd->_.src = src;//14
	}
		break;
	case E_ADDR:
	case E_MARK:;
	{
		reg_t src = {};
		if (!check_common_regno_2 (loc, &loc, &src)) {
			PRINT ("Value: %c\n", *loc);
			PANIC ("2-nd operand of STORE should be common register!\n\"%s\"\n", start);
			return EXIT_FAILURE;
		}
		cmd->_.src = src;
		cmd->_._type = C_STORE;
		if (ent._type == E_ADDR) {
			cmd->_.param = ent.address;
		} else {
			cmd->mark_id = ent.mark_id;
		}
		break;//15
	}
	default:
		PANIC ("first entity should be register, mark or address\n\"%s\"", start);
		return EXIT_FAILURE;
	}
	if (*loc != COMMENT && *loc != 0) {
		PANIC ("something unuseful in command\n\"%s\"\n", start);
		return EXIT_FAILURE;
	}

	if (*loc == COMMENT) {
		cmd->comment = loc + 1;
		cmd->comment_length = length - ((loc + 1) - start);
	} else {
		cmd->comment = "";
		cmd->comment_length = 0;
	}
	return EXIT_SUCCESS;
}

int parse (state_t* state, parsed_t* prog, bool extended_mode) {
	//1: detect commands
	prog->cmd_count = 1;

	for (const char* p = state->buffer; *p; ++p) { prog->cmd_count += (*p == LF); }
	const unsigned commands = prog->cmd_count;
#define ALLOC_END(member, type, count, note) \
	member = alloc(type, count); \
	if (member == NULL) { \
		PRINT ("Couldn't allocate %lu bytes for " note " (%u objects)\n", size_calc(type, count), count); \
		return EXIT_FAILURE; \
	} \
	PRINT ("Allocated %lu bytes for " note " (%u objects)\n", size_calc(type, count), count); \
	memset (member, 0, size_calc(type, count)); 
	ALLOC_END(prog->commands, command_ext_t, commands, "commands");
	//2: detect marks
	const char* p = state->buffer;
	while (*p) {
		const char* loc = p, *ptr;
		reg_t _reg;
		if (!isdigit (loc[0]) && is_valid (loc[0]) && 
			(!maybe_register (loc, &ptr, &_reg) || is_valid (*ptr) || *ptr == MARK_DEF)) {
			if (prog->m_count == 0xFF) {
				PANIC ("too many marks\n");
				return EXIT_FAILURE;
			}
			++prog->m_count;
			if (extended_mode) {
				parsed_ext_t* prg_ext = (parsed_ext_t*) prog;
				if (*p == LOAD && p[1] == ARG_OPEN) {
					++prg_ext->l_count;
				} else if (*p == STORE && p[1] == ARG_OPEN) {
					++prg_ext->s_count;
				} else if (*p == ADDR && p[1] == MARK_DEF) {
					++prg_ext->p_count;
				} else if (*p == JUMP && p[1] == MARK_DEF) {
					++prg_ext->j_count;
				} else if (*p == NEXT && p[1] == MARK_DEF) {
					++prg_ext->n_count;
				}
			}
		}
		while (*p && *p != LF) ++p;
		if (*p == LF) ++p;
	}
	ALLOC_END(prog->marks, mark_info_t, prog->m_count, "marks");
	if (extended_mode) {
		parsed_ext_t* prg_ext = (parsed_ext_t*) prog;
		if (prg_ext->l_count) {
			ALLOC_END(prg_ext->l_marks, mark_param_t, prg_ext->l_count, "loader marks");
		}
		if (prg_ext->s_count) {
			ALLOC_END(prg_ext->s_marks, mark_param_t, prg_ext->s_count, "store marks");
		}
		if (prg_ext->p_count) {
			ALLOC_END(prg_ext->p_marks, word, prg_ext->p_count, "addresses marks");
		}
		if (prg_ext->j_count) {
			ALLOC_END(prg_ext->j_marks, word, prg_ext->j_count, "jumps marks");
		}
		if (prg_ext->n_count) {
			ALLOC_END(prg_ext->n_marks, word, prg_ext->n_count, "nexts marks");
		}
	}
#undef ALLOC_END
	p = state->buffer;
	//3: process commands
	local_state st = {0, 0, 0, 0, 0, 0};
	const word limit = prog->m_count;
	bool prev_0 = false;
	unsigned _i = 0;
	for (unsigned i = 0; i < commands; ++i) {
		const char* from = shorted.buffer + shorted.length;
		const unsigned len_old = shorted.length;
		shortify (&p);
		const unsigned len = shorted.length - len_old;
		if (len == 0 && prev_0) {
			continue;
		}
		prev_0 = (len == 0);
		if (check (prog, extended_mode, &st, _i, from, shorted.length - len_old) == EXIT_FAILURE) {
			PRINT ("Trace: \"%s\"\n", from);
			for (word i = 0; i < st.m_cnt; ++i) {
				const mark_info_t* info = prog->marks + i;
				printf ("mark_info_t = { name=\"%.*s\", cmd_id=%u }\n", info->mark_len, info->name, info->cmd_id);
			}
			return EXIT_FAILURE;
		}
		const command_ext_t* cmd = prog->commands + _i;
		const word m_id = cmd->mark_id;
		const unsigned length = (m_id == limit ? 4 : prog->marks[m_id].mark_len);
		const char* const name = ((m_id == limit) ? "None" : prog->marks[m_id].name);
		printf ("Parsed command_ext[%u] { _ = { dst=%u, src=%u, param=%u, type=%s}, comment=\"%.*s\", mark=\"%.*s\"}\n",
			_i,
			cmd->_.dst, cmd->_.src, cmd->_.param, cmd_names[cmd->_._type], cmd->comment_length, cmd->comment, 
			length, name);

		++_i;
	}
	prog->cmd_real = _i;
	PRINT ("Shorted length: %u\n", _i);
	PRINT ("Marks:\n");
#define M_CHECK(where, type, note) \
	if (where->type ## _count != st.type ## _cnt) { \
		PANIC ("found %u" note "marks, should be %u\n", st.type ## _cnt, where->type ##_count); \
		return EXIT_FAILURE; \
	}
	M_CHECK(prog, m, " ");
	if (extended_mode) {
		parsed_ext_t* _prog = (parsed_ext_t*) prog;
		M_CHECK(_prog, j, " jump ");
		M_CHECK(_prog, p, " addres ");
		M_CHECK(_prog, l, " load ");
		M_CHECK(_prog, s, " store ");
		M_CHECK(_prog, n, " nexts ");
	}
#undef M_CHECK
	for (word i = 0; i < st.m_cnt; ++i) {
		const mark_info_t* info = prog->marks + i;
		printf ("mark_info_t = { name=\"%.*s\", cmd_id=%u }\n", info->mark_len, info->name, info->cmd_id);
	}
	return EXIT_SUCCESS;
}
