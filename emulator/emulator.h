#ifndef EMULATOR_COMMON_H_
#define EMULATOR_COMMON_H_

#include "../common/common.h"
#include <ncurses.h>

typedef struct {
	word buffer[0x100];
	word registers[RF + 1];
} machine_t;

typedef enum : word {
	E_OK = 0,
	E_DIVZERO = 1,
	E_UNKNOWN = 2,
	E_LOOP = 3 // HLT: RC <- @HLT
} error_t;
#define BACKGROUND COLOR_BLACK
typedef enum : word {
	GREY = 0, //grey on BACKGROUND
	WHITE = 1, //white on BACKGROUND
	RED = 2, //red on BACKGROUND
	BLUE = 3, //blue on BACKGROUND
	ERROR = 4 // red on white
} color_t;

extern unsigned width;

extern char line[80];

int get_command (command_t* cmd, const machine_t* state, word rc);
error_t step (machine_t* machine);

int run_emulator (machine_t* machine);

extern WINDOW *registers, *program, *text, *input;
int init (machine_t* machine);

bool init_random (void);
void cleanup_random (void);

void clear_resources (void);

bool read_line (void);

//On emulator
void mark_byte (word address, machine_t* machine, color_t color);

#endif
