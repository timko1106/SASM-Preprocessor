#ifndef COMMON_H_
#define COMMON_H_

#include <stddef.h>
#include <stdint.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include <ctype.h>

#define PRINT(text, ...) printf ("["__FILE__":%d]: " text, __LINE__, ##__VA_ARGS__)
#define PANIC(text, ...) printf ("ERROR[" __FILE__ ":%d]: " text, __LINE__, ##__VA_ARGS__)

//Debug print:
#ifdef DEBUG

#define PRINT_D(text, ...) printf ("["__FILE__":%d]: " text, __LINE__, ##__VA_ARGS__)

#else
#define PRINT_D(text, ...)
#endif


//VAL1 <- VAL2 ...
#define SEND_1 '<'
#define SEND_2 '-'

#define LF '\n'
#define MARK '@'
#define MARK_DEF ':'

#define COMMENT ';'

#define REGISTERS RC

#define REF_OPEN '['
#define REF_CLOSE ']'

//binary register encoding NOT SUPPORTED!

#define ARG_OPEN '('
#define ARG_CLOSE ')'

//Start of register identifier
#define R_START 'R'
#define r_START 'r'

#define REG_CMP 'F'
#define REG_JMP 'C'

#define J_CARRY 'C'
#define J_SIGN 'S'
#define J_ZERO 'Z'

#define CMP_SIGN '~'
#define RAND '?'

//Initialization operands:
//All marks L - Load register from
//Example:
//L(1): R0 <- R0 - load to register 1
#define LOAD 'L'
//All marks S - Store register to
//Example:
//S(1): R0 <- R0 - store register 1
#define STORE 'S'
//All marks J - unconditional jump
#define JUMP 'J'

//All marks P - addresses to commands
#define ADDR 'P'
//All marks N - addresses to next byte after first byte of command
#define NEXT 'N'

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
	C_JMP_C = 9, // Jump on carry flag
	C_JMP_S = 10,// Jump on sign  flag
	C_JMP_Z = 11,// Jump on zero  flag
	C_JMP 	= 12,
	C_RAND 	= 13,
	C_INC 	= 14,
	C_NONE 	= 15
} cmd_t;//C_NONE can be used as just comment.

typedef enum : word {
	R0 = 0,
	R1 = 1,
	R2 = 2,
	R3 = 3,
	RC = 4,
	RF = 5
} reg_t;


extern const word sizes[16];

extern const char* const cmd_names[16];

typedef struct {
	cmd_t _type;
	word src, dst;//Source and destination registers
	word param;
} command_t;

typedef struct {
	command_t _;
	const char* comment; // To store after preprocessing.
	unsigned comment_length;//На самом деле указывают на препроцессированный буффер...
	word mark_id; // mark identifier for C_MOV.
	//All marks would get their identifiers (no reason to store here).
} command_ext_t;

typedef struct {
	const char* name;
	unsigned cmd_id, mark_len;
	word addr;//addr - address in program.
} mark_info_t;

typedef struct {
	unsigned cmd_count, cmd_real;
	command_ext_t* commands;
	mark_info_t* marks;
	word m_count;
} parsed_t;

typedef struct {
	word mark_id;
	word reg;//register
} mark_param_t;

typedef struct {
	parsed_t _;
	//Loads, Stores, Addresses, Jumps and Nexts count
	word l_count, s_count, p_count, j_count, n_count;
	mark_param_t *l_marks, *s_marks;
	word *p_marks, *j_marks, *n_marks;
} parsed_ext_t;

//8KB хватит всем
//4KB внезапно не хватило (из-за комментариев)
#define LIMIT 8192

typedef struct {
	char buffer[LIMIT];
	size_t length;
} state_t;

//[_0-9A-Za-z]
bool is_valid (char);

//Memcpy with check size=0
void memcpy_s (char* dst, const char* src, size_t size);

//Doesn't call free(program)! Should be on stack!
void data_parsed_free (parsed_t* program);
void data_parsed_ext_free (parsed_ext_t* program);

//extended -> allocated as parsed_ext_t, else as parsed_t.
//Проходов будет много. ОЧЕНЬ МНОГО.
int parse (state_t* state, parsed_t* program, bool extended_mode);

//linker - all marks should get their addresses (in final program).
//Commands get addresses of marks.
int linker (parsed_t* program, word offset);

unsigned disassemble (const command_t* cmd, bool is_extended, char* line, word m_count, mark_info_t* marks);

#define size_calc(type, count) (sizeof(type) * ((size_t)(count)) )
#define alloc(type, count) (type*) malloc(size_calc (type, count))

#endif
