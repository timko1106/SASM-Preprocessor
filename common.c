#include "common.h"

bool is_valid (char x) {
	return isalpha (x) || isdigit (x) || x == '_';
}

uint8_t sizes[16] = {
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
const char* cmd_names[16] = {
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
