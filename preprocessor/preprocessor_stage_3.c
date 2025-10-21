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
//#define DEBUG

int stage_3 (state_t* state_new, parsed_ext_t* prog, bool need_init) {
	//Special insertion:
	//R3 <- 0xFF ; 0 1
	//4 bytes on each prog->j_cnt, prog->l_cnt, prog->s_cnt
	//RC <- @_start ; +2 bytes
	word offset = (need_init ? (2 + 4 * (prog->j_count + prog->l_count + prog->s_count) + 2) : 0);
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

#define LENGTH state_new->length
#define BUFFER state_new->buffer + LENGTH
#define B_NEXT state_new->buffer[LENGTH++]
	state_new->buffer[0] = 0;
	LENGTH = 0;

	if (need_init) {
		const char START[] = "\tR3 <- 0xFF; 0 1\n_init: ";
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
	}
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
		if (_type == C_NONE) {
			if (cmd->comment_length != 0) {
				LENGTH += sprintf (BUFFER, 
					";%.*s",
					cmd->comment_length, cmd->comment);
			}
		} else {
			LENGTH += disassemble ((command_t*) cmd, true, BUFFER, m_count, marks);
			LENGTH += sprintf (BUFFER, 
					" ; ("
#ifdef DEBUG
					"cmd_id=%u; "
#endif
					"%u",
#ifdef DEBUG
					i,
#endif
					offset);
			if (sizes[_type] == 2) {
				LENGTH += sprintf (BUFFER, ", %u", offset + 1);
			}
			B_NEXT = ')';
			if (cmd->comment_length != 0) {
				LENGTH += sprintf (BUFFER, " %.*s", cmd->comment_length, cmd->comment);
			}
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
