#include "preprocessor.h"
#include <stdlib.h>

typedef struct {
	word mark_id;
	unsigned cmd_id;
} mark_p;
static int cmp (const void* l, const void* r) {
	const mark_p* L = (const mark_p*)l;
	const mark_p* R = (const mark_p*)r;

	return (L->cmd_id < R->cmd_id || (L->cmd_id == R->cmd_id && L->mark_id < R->mark_id)) ? -1 : 1;
}

int stage_3 (state_t* state_new, parsed_ext_t* prog) {
	//Special insertion:
	//R3 <- 0xFF ; 0 1
	//4 bytes on each prog->j_cnt, prog->l_cnt, prog->s_cnt
	//RC <- @_start ; +2 bytes
	word offset = 2 + 4 * (prog->j_count + prog->l_count + prog->s_count) + 2;
	if (linker (&prog->_, offset) == EXIT_FAILURE) {
		return EXIT_FAILURE;
	}

	const word m_count = prog->_.m_count;
	mark_p* ordered = alloc (mark_p, m_count);
	if (ordered == NULL) {
		PANIC ("Couldn't allocate %lu bytes (%u blocks)\n",
			size_calc (mark_p, m_count), m_count);
		return EXIT_FAILURE;
	}
	PRINT ("Allocated %lu bytes (%u blocks)\n",
		size_calc (mark_p, m_count), m_count);
	
	mark_info_t* marks = prog->_.marks;
	command_ext_t* cmds = prog->_.commands;
	const unsigned cmd_count = prog->_.cmd_real;
	for (word i = 0; i < m_count; ++i) {
		ordered[i].mark_id = i;
		ordered[i].cmd_id = marks[i].cmd_id;
	}
	qsort (ordered, m_count, sizeof(mark_p), cmp);

	state_new->buffer[0] = 0;
	const char START[] = "\tR3 <- 0xFF; 0 1\n_init: ";
#define LENGTH state_new->length
#define BUFFER state_new->buffer + LENGTH
#define B_NEXT state_new->buffer[LENGTH++]

	memcpy (state_new->buffer, START, sizeof (START) - 1);
	LENGTH = sizeof(START) - 1;
	static const char hex[] = "0123456789ABCDEF";
#define MARK_SPEC_2(type, description, opcode, mult) \
	for (word i = 0; i < prog->type ## _count; ++i) { \
		const mark_info_t* info = marks + prog->type ## _marks[i].mark_id; \
		LENGTH +=  sprintf (BUFFER, \
			"R0 <- 0x%c%c; " description " register %u at cmd_id=%u\n\t@%u <- R0;\n\t", \
			hex[opcode], hex[prog->type ## _marks[i].reg * mult], \
			prog->type ## _marks[i].reg, info->cmd_id, info->addr); \
	}
	for (word i = 0; i < prog->j_count; ++i) { 
		const mark_info_t* info = marks + prog->j_marks[i]; 
		LENGTH += sprintf (BUFFER, 
				"R0 <- 0x%c0; JUMP at cmd_id=%u\n\t@%u <- R0;\n\t",
				hex[C_JMP], info->cmd_id, info->addr); 
	}
	MARK_SPEC_2(l, "LOAD", C_LOAD, 4);
	MARK_SPEC_2(s, "STORE", C_STORE, 1);
#undef MARK_SPEC_2

	const char END[] = "RC <- @_start;";

	memcpy (BUFFER, END, sizeof (END) - 1);
	LENGTH += sizeof (END) - 1;
	word j = 0;
	for (unsigned i = 0; i < cmd_count; ++i) {
		B_NEXT = '\n';
		bool has_mark = false;
		if (j < m_count && i == ordered[j].cmd_id) {
			const word m_id = ordered[j].mark_id;
			const char* name = marks[m_id].name;
			if (! (marks[m_id].mark_len == 1 && 
				(*name == LOAD || *name == STORE || *name == JUMP 
				|| *name == ADDR || *name == NEXT)) ) {
				LENGTH += sprintf (BUFFER, 
					"%.*s: ", marks[m_id].mark_len, name);
				has_mark = true;
			}
			++j;
		}
		const command_ext_t* cmd = cmds + i;
		const cmd_t _type = cmd->_._type;
		if (_type != C_NONE && !has_mark) {
			B_NEXT = '\t';
		}
		switch (_type) {
		case C_NONE:
			LENGTH += sprintf (BUFFER, 
				"; %.*s", cmd->comment_length, cmd->comment);
			break;
		case C_MOV:
			LENGTH += sprintf (BUFFER, 
				"R0%u <- R0%u ; (cmd_id=%u; %u) %.*s",
				cmd->_.dst, cmd->_.src, i, offset, cmd->comment_length, cmd->comment);
			break;
		case C_MOV_V:
			LENGTH += sprintf (BUFFER, 
				"R0%u <- %u; (cmd_id=%u; %u, %u) %.*s",
				cmd->_.dst, cmd->_.param, i, offset, offset + 1, cmd->comment_length, cmd->comment);
			break;
		case C_LOAD:
			LENGTH += sprintf (BUFFER, 
				"R0%u <- @%u ; (cmd_id=%u; %u, %u) %.*s",
				cmd->_.dst, cmd->_.param, i, offset, offset + 1, cmd->comment_length, cmd->comment);
			break;
		case C_STORE:
			LENGTH += sprintf (BUFFER, 
				"@%u <- R0%u ; (cmd_id=%u; %u, %u) %.*s",
				cmd->_.param, cmd->_.src, i, offset, offset + 1, cmd->comment_length, cmd->comment);
			break;
		case C_ADD:
			LENGTH += sprintf (BUFFER, 
				"R0%u <- R0%u + R0%u ; (cmd_id=%u; %u) %.*s",
				cmd->_.dst, cmd->_.dst, cmd->_.src, i, offset, cmd->comment_length, cmd->comment);
			break;
		case C_SUB:
			LENGTH += sprintf (BUFFER, 
				"R0%u <- R0%u - R0%u ; (cmd_id=%u; %u) %.*s",
				cmd->_.dst, cmd->_.dst, cmd->_.src, i, offset, cmd->comment_length, cmd->comment);
			break;
		case C_MUL:
			LENGTH += sprintf (BUFFER, 
				"R0%u <- R0%u * R0%u ; (cmd_id=%u; %u) %.*s",
				cmd->_.dst, cmd->_.dst, cmd->_.src, i, offset, cmd->comment_length, cmd->comment);
			break;
		case C_DIV:
			LENGTH += sprintf (BUFFER, 
				"R0%u <- R0%u / R0%u ; (cmd_id=%u; %u) %.*s",
				cmd->_.dst, cmd->_.dst, cmd->_.src, i, offset, cmd->comment_length, cmd->comment);
			break;
		case C_CMP:
			LENGTH += sprintf (BUFFER, 
				"RF <- R0%u ~ R0%u ; (cmd_id=%u; %u) %.*s",
				cmd->_.dst, cmd->_.src, i, offset, cmd->comment_length, cmd->comment);
			break;
		case C_JMP_C:
		case C_JMP_S:
		case C_JMP_Z:
		case C_JMP:;
			static const char buff[] = "CSZ";
			static const char BEGIN[] = "RC <- @";
			memcpy (BUFFER, BEGIN, sizeof (BEGIN) - 1);
			LENGTH += sizeof (BEGIN) - 1;
			if (cmd->mark_id == m_count) {
				LENGTH += sprintf (BUFFER, 
					"%u ", cmd->_.param);
			} else {
				const mark_info_t* info = marks + cmd->mark_id;
				memcpy (BUFFER, info->name, info->mark_len);
				LENGTH += info->mark_len;
			}
			if (_type != C_JMP) {
				B_NEXT = '(';
				B_NEXT = buff[_type - C_JMP_C];
				B_NEXT = ')';
			}
			LENGTH += sprintf (BUFFER,
				"; (cmd_id=%u; %u, %u) %.*s",
				i, offset, offset + 1, cmd->comment_length, cmd->comment);

			break;
		case C_RAND:
			LENGTH += sprintf (BUFFER, 
				"R0%u <- ? ; (cmd_id=%u; %u) %.*s",
				cmd->_.dst, i, offset, cmd->comment_length, cmd->comment);
			break;
		case C_INC:
			LENGTH += sprintf (BUFFER, 
				"R0%u <- R0%u++ ; (cmd_id=%u; %u) %.*s",
				cmd->_.dst, cmd->_.src, i, offset, cmd->comment_length, cmd->comment);
			break;
		}
		PRINT("Length: %zu, cmd_id: %u\n", LENGTH, i);
		offset += sizes[_type];
	}
	state_new->buffer[LENGTH] = 0;
#undef LENGTH
#undef BUFFER
#undef B_NEXT
	free (ordered);
	return EXIT_SUCCESS;
}
