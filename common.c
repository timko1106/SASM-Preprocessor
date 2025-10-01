#include "common.h"

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
		[C_NONE] = 0 };

void memcpy_s (char* dst, const char* src, size_t size) {
	if (size == 0) return;
	memcpy (dst, src, size);
}
