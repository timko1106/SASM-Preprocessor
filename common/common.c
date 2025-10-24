#include "common.h"

unsigned disassemble (const command_t* cmd, bool is_extended, char* line, word m_count, mark_info_t* marks) {
	unsigned len = 0;
	switch (cmd->_type) {
	case C_NONE:
		break;
	case C_MOV:
		len = sprintf (line, 
			"R0%u <- R0%u",
			cmd->dst, cmd->src);
		break;
	case C_MOV_V:
		len = sprintf (line, 
			"R0%u <- %u",
			cmd->dst, cmd->param);
		break;
	case C_LOAD:
		len = sprintf (line, 
			"R0%u <- @%u",
			cmd->dst, cmd->param);
		break;
	case C_STORE:
		len = sprintf (line, 
			"@%u <- R0%u",
			cmd->param, cmd->src);
		break;
	case C_ADD:
	case C_SUB:
	case C_MUL:
	case C_DIV:;
		static const char operations[] = "+-*/";
		len = sprintf (line, 
			"R0%u <- R0%u %c R0%u",
			cmd->dst, cmd->dst, operations[cmd->_type - C_ADD], cmd->src);
		break;
	case C_CMP:
		len = sprintf (line, 
			"RF <- R0%u ~ R0%u",
			cmd->dst, cmd->src);
		break;
	case C_JMP_C:
	case C_JMP_S:
	case C_JMP_Z:
	case C_JMP:;
		static const char buff[] = "CSZ";
		static const char BEGIN[] = "RC <- @";
		memcpy (line, BEGIN, sizeof (BEGIN) - 1);
		len = sizeof (BEGIN) - 1;
		if (!is_extended || ((command_ext_t*)cmd)->mark_id == m_count) {
			len += sprintf (line + len, 
				"%u ", cmd->param);
		} else {
			const mark_info_t* info = marks + ((command_ext_t*)cmd)->mark_id;
			memcpy (line + len, info->name, info->mark_len);
			len += info->mark_len;
		}
		if (cmd->_type != C_JMP) {
			line[len++] = '(';
			line[len++] = buff[cmd->_type - C_JMP_C];
			line[len++] = ')';
		}
		line[len] = 0;
		break;
	case C_RAND:
		len = sprintf (line, 
			"R0%u <- ?",
			cmd->dst);
		break;
	case C_INC:
		len = sprintf (line, 
			"R0%u <- R0%u++",
			cmd->dst, cmd->src);
		break;
	}
	return len;
}



bool is_valid (char x) {
	return isalpha (x) || isdigit (x) || x == '_';
}

//sizes
const uint8_t sizes[16] = {
		[C_MOV] = 1,
		[C_LOAD] = 2,
		[C_STORE] = 2,
		[C_MOV_V] = 2,
		[C_ADD] = 1,
		[C_SUB] = 1,
		[C_MUL] = 1,
		[C_DIV] = 1,
		[C_CMP] = 1,
		[C_JMP_C] = 2,
		[C_JMP_S] = 2,
		[C_JMP_Z] = 2,
		[C_JMP] = 2,
		[C_RAND] = 1,
		[C_INC] = 1,
		[C_NONE] = 0 };
#define Q(p) [p] = #p
const char* const cmd_names[16] = {
		Q(C_MOV),
		Q(C_LOAD),
		Q(C_STORE),
		Q(C_MOV_V),
		Q(C_ADD),
		Q(C_SUB),
		Q(C_MUL),
		Q(C_DIV),
		Q(C_CMP),
		Q(C_JMP_C),
		Q(C_JMP_S),
		Q(C_JMP_Z),
		Q(C_JMP),
		Q(C_RAND),
		Q(C_INC),
		Q(C_NONE)};

#undef Q

void memcpy_s (char* dst, const char* src, size_t size) {
	if (size == 0) return;
	memcpy (dst, src, size);
}

#define FREE(member) \
if (member != NULL) { \
	free (member); \
	member = NULL; \
}

void data_parsed_free (parsed_t* program) {
	FREE(program->commands);
	FREE(program->marks);
}

void data_parsed_ext_free (parsed_ext_t* program) {
	data_parsed_free ((parsed_t*) program);
	FREE(program->l_marks);
	FREE(program->p_marks);
	FREE(program->s_marks);
	FREE(program->j_marks);
	FREE(program->n_marks);
}

#undef FREE
