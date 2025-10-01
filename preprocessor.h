#ifndef PREPROCESSOR_H_
#define PREPROCESSOR_H_

#include <stdlib.h>

#include "common.h"
#include <stddef.h>

//4KB хватит всем
#define LIMIT 4096

#define MARK '@'

//# should be FIRST symbol of the line. Bracket or space should be next after name.
#define MACRO_DEF '#'
//% can be just anywhere.
#define MACRO_USE '%'
#define MACRO_CONTINUE '\\'
#define LF '\n'

#define ARG_OPEN '('
#define ARG_OPEN2 "("
#define ARG_CLOSE ')'
//Converted to %\0, %\1 %\2... %\255
#define ARG_LEN 2
//Arguments' separator
#define ARG_SEP ','

//Initialization operands:
//All marks L - Load register from
//Example:
//L(1): R0 <- R0 - load to register 1
#define LOAD 'L'
//All marks S - Store register to
//Example:
//S(1): R0 <- R0 - store register 1
#define STORE 'S'

//All marks P - addresses to write register to
#define ADDR 'P'


#define RSP "3"
#define rsp "rsp"

typedef struct {
	char buffer[LIMIT];
	size_t length;
} state_t;

typedef struct {
	command_t _;
	const char* comment; // To store after preprocessing.
	uint8_t mark_id; // mark identifier for C_MOV.
	//All marks would get their identifiers (no reason to store here).
} command_ext_t;

int preprocess (state_t* state, int fd_out);

//Stage 1: macro expanding
//Also adds line #rsp R03
int stage_1 (state_t* state, state_t* state_new);
//State 2: compilation, marks indexing
int stage_2 (state_t* state, command_ext_t** commands, char*** marks);
//Stage 3: marks setting for MOV-es, final output
int stage_3 (state_t* state, command_ext_t* commands, char** marks);

#endif
