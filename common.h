#ifndef COMMON_H_
#define COMMON_H_

#include <stddef.h>
#include <stdint.h> 
#include <stdio.h>
#include <string.h>

typedef uint8_t word;

typedef enum : word {
	C_MOV 	= 0,
	C_LOAD 	= 1, // Load from RAM
	C_STORE = 2, // Store into RAM
	C_MOV_V = 3,
	C_ADD 	= 4,
	C_SUB 	= 5,
	C_MUL 	= 6,
	C_DIV 	= 7,
	C_CMP 	= 8,
	C_JMP_C = 9,
	C_JMP_S = 10,
	C_JMP_Z = 11,
	C_JMP 	= 12,
	C_RAND 	= 13,
	C_INC 	= 14,
	C_NONE 	= 15
} cmd_t;//C_NONE can be used as just comment.

extern uint8_t sizes[16];

typedef struct {
	cmd_t _type;
	word src, dst;//Source and destination registers
	word param;
} command_t;

void memcpy_s (char* dst, const char* src, size_t size);
#define size_calc(type, count) (sizeof (type) * count)
#define alloc(type, count) malloc(size_calc (type, count))

#endif
