#ifndef PREPROCESSOR_H_
#define PREPROCESSOR_H_

#include <stdlib.h>

#include "common.h"
#include <stddef.h>



//# should be FIRST symbol of the line. Bracket or space should be next after name.
#define MACRO_DEF '#'
//% can be just anywhere.
#define MACRO_USE '%'
#define MACRO_CONTINUE '\\'

#define ARG_OPEN2 "("
//Converted to %\0, %\1 %\2... %\255
#define ARG_LEN 2
//Arguments' separator
#define ARG_SEP ','



#define RSP "3"
#define rsp "rsp"



int preprocess (state_t* state, int fd_out);

//Stage 1: macro expanding
//Also adds line #rsp R03
int stage_1 (state_t* state, state_t* state_new);
//State 2: compilation, marks indexing
int stage_2 (state_t* state, parsed_ext_t* prog);
//Stage 3: marks setting for MOV-es, final output
int stage_3 (state_t* state, state_t* state_new, parsed_ext_t* prog);

#endif
