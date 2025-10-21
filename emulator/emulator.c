#include "emulator.h"
#include <locale.h>
#include <errno.h>
#include <unistd.h>

static const unsigned PROG_WIDTH = 2 + 3 * (16 + 1);
static const unsigned PROG_HEIGHT = 1 + 16;
//R0: [2 symbols each bit] + 1 bit space
static const unsigned REG_WIDTH = (4 + 8 * 2 + 1);
static const unsigned REG_HEIGHT = 1 + 2 * (RF + 1);
static const unsigned REG_START = (PROG_HEIGHT - REG_HEIGHT) / 2;
static const unsigned INSTRUCTIONS = 2;//2 up, current and 2 down
static const unsigned FULL = 2 * INSTRUCTIONS + 1;

static const unsigned FPS = 4;
static const useconds_t DELTA = (((useconds_t)1000) * 1000) / FPS;

//1 line horizontal
//1 line Instructions:
//FULL lines of instructions
//1 line Output:
//1 line of text
//1 line horizontal line
static const unsigned TEXT_HEIGHT = FULL + 5;
static const unsigned INPUT_HEIGHT = 3;

WINDOW *registers = NULL, *program = NULL, *text = NULL, *input = NULL;
unsigned width = 0;
char line[80] = {};

bool read_line (void) {
	return getnstr (line, sizeof (line)) == OK;
}

static char lines[5][30] = {};
static char* ptrs[5] = {lines[0], lines[1], lines[2], lines[3], lines[4]};

static void disassembling (const machine_t* machine) {
	char* const old_0 = ptrs[0];
	for (unsigned i = 0; i < INSTRUCTIONS; ++i) {
		ptrs[i] = ptrs[i + 1];
	}
	ptrs[INSTRUCTIONS] = old_0;
	word rc = machine->registers[RC];
	command_t cmd = {};
	for (unsigned i = 0; i <= INSTRUCTIONS; ++i) {
		if (get_command (&cmd, machine, rc) == EXIT_FAILURE) {
			ptrs[INSTRUCTIONS + i][0] = 0;
			sprintf (ptrs[INSTRUCTIONS + i], "; UNKNOWN %02x", rc);
			continue;
		}
		const unsigned len = disassemble (&cmd, false, ptrs[INSTRUCTIONS + i], 0, NULL);
		sprintf (ptrs[INSTRUCTIONS + i] + len, " ; %02x", rc);
		rc += sizes[cmd._type];
	}
	wattrset (text, A_BOLD | COLOR_PAIR(WHITE));
	for (unsigned i = 0; i < FULL; ++i) {
		if (i == INSTRUCTIONS) {
			wattrset (text, A_BOLD | COLOR_PAIR(RED));
		}
		wmove (text, 2 + i, 0);
		wclrtoeol (text);
		mvwprintw (text, 2 + i, 0, "%s", ptrs[i]);
		if (i == INSTRUCTIONS) {
			wattrset (text, A_BOLD | COLOR_PAIR(WHITE));
		}
	}
	wrefresh (text);
}

static word red_pos = 0;
static bool red_update = false;

void mark_byte (word address, machine_t* machine, color_t color) {
	const word i = address / 16, j = address % 16;
	wattrset (program, COLOR_PAIR(color));
	if (color == RED) {
		red_pos = address;
		red_update = true;
	}
	if (color == GREY) {
		wattron (program, A_DIM);
	} else {
		wattron (program, A_BOLD);
	}
	mvwprintw (program, 1 + i, 5 + j * 3, "%02x", machine->buffer[i * 16 + j]);
}

static void print_registers (machine_t* machine) {
	wattrset (registers, A_BOLD);
	for (unsigned i = 0; i <= RF; ++i) {
		const word reg = machine->registers[i];
		wattron (registers, COLOR_PAIR(WHITE));
		mvwprintw (registers, 1 + i * 2, 3, " %d %d %d %d %d %d %d %d",
			(bool)(reg & 0x80),
			(bool)(reg & 0x40),
			(bool)(reg & 0x20),
			(bool)(reg & 0x10),
			(bool)(reg & 0x08),
			(bool)(reg & 0x04),
			(bool)(reg & 0x02),
			(bool)(reg & 0x01)
		);
	}
}

int init (machine_t* machine) {
	initscr ();
	if (!has_colors ()) {
		PANIC("Colors not supported!\n");
		return EXIT_FAILURE;
	}
	start_color ();
	init_pair (WHITE, COLOR_WHITE, BACKGROUND);
	init_pair (RED, COLOR_RED, BACKGROUND);
	init_pair (BLUE, COLOR_BLUE, BACKGROUND);
	init_pair (ERROR, COLOR_RED, COLOR_WHITE);
	
	unsigned row, col;
	getmaxyx(stdscr, row, col);
	if (row < PROG_HEIGHT + TEXT_HEIGHT + INPUT_HEIGHT || col < REG_WIDTH + PROG_WIDTH) {
		PANIC ("Too small window size! Need at least %u * %u (width * height), actual %u * %u\n", REG_WIDTH + PROG_WIDTH, PROG_HEIGHT + 1 + TEXT_HEIGHT + 1 + INPUT_HEIGHT, col, row);
		return EXIT_FAILURE;
	}
	if (!init_random ()) {
		return EXIT_FAILURE;
	}

	setlocale(LC_ALL, "");

#define CHECK_WINDOW(name, height, width, base_y, base_x) \
	name = newwin (height, width, base_y, base_x); \
	if (name == NULL) { \
		PANIC ("ERROR: Can't create window " #name "\n"); \
		return EXIT_FAILURE; \
	}
	CHECK_WINDOW (registers, REG_HEIGHT, REG_WIDTH, REG_START, 0);
	CHECK_WINDOW (program, PROG_HEIGHT, PROG_WIDTH, 0, REG_WIDTH);
	CHECK_WINDOW (text, TEXT_HEIGHT, col, PROG_HEIGHT, 0);
	CHECK_WINDOW (input, INPUT_HEIGHT, col, PROG_HEIGHT + TEXT_HEIGHT, 0);
#undef CHECK_WINDOW
	width = col;
	refresh ();
	const char* reg_names[] = {"R0", "R1", "R2", "R3", "RC", "RF"};
	wattrset (registers, A_BOLD);
	for (unsigned i = 0; i <= RF; ++i) {
		wattron (registers, COLOR_PAIR(RED));
		mvwprintw (registers, 1 + i * 2, 0, "%s:", reg_names[i]);
		wattroff (registers, COLOR_PAIR(RED));
		wattron (registers, COLOR_PAIR(WHITE));
		mvwprintw (registers, 1 + i * 2, 3, " 0 0 0 0 0 0 0 0");
	}
	mvwhline (registers, 0, 0, ACS_HLINE, REG_WIDTH);
	mvwhline (registers, REG_HEIGHT - 1, 0, ACS_HLINE, REG_WIDTH);
	wrefresh (registers);

	wattron (program, COLOR_PAIR(WHITE) | A_BOLD);
	mvwvline (program, 0, 0, ACS_VLINE, PROG_HEIGHT/* - 1*/);
	//mvwhline (program, PROG_HEIGHT - 1, 1, ACS_HLINE, PROG_WIDTH - 1);
	//mvwaddch (program, PROG_HEIGHT - 1, 0, ACS_LLCORNER);
	mvwaddch (program, REG_START, 0, ACS_RTEE);
	mvwaddch (program, REG_START + REG_HEIGHT - 1, 0, ACS_RTEE);
	for (unsigned i = 0; i < 16; ++i) {
		mvwprintw (program, 0, 5 + i * 3, "%02x", i);
	}
	wattrset (program, A_NORMAL);
	for (unsigned i = 0; i < 16; ++i) {
		wattron (program, COLOR_PAIR(WHITE) | A_BOLD);
		mvwprintw (program, 1 + i, 1, " %02x ", i);
		wattrset (program, A_NORMAL);
		wattron (program, COLOR_PAIR(GREY) | A_DIM);
		for (unsigned j = 0; j < 16; ++j) {
			mvwprintw (program, 1 + i, 5 + j * 3, "%02x", machine->buffer[i * 16 + j]);
		}
		wattrset (program, A_NORMAL);
	}
	wrefresh (program);
	wattron (text, COLOR_PAIR(WHITE) | A_BOLD);
	mvwhline (text, 0, 0, ACS_HLINE, width);
	mvwaddch (text, 0, REG_WIDTH, ACS_BTEE);
	mvwprintw (text, 1, 0, "Instructions:");
	mvwprintw (text, 1 + FULL + 1, 0, "Output:");
	mvwhline (text, TEXT_HEIGHT - 1, 0, ACS_HLINE, width);
	wrefresh (text);
	wattron (input, COLOR_PAIR(WHITE) | A_BOLD);
	mvwprintw (input, 0, 0, "Input:");
	mvwprintw (input, 1, 0, "> ");
	mvwhline (input, INPUT_HEIGHT - 1, 0, ACS_HLINE, width);
	wrefresh (input);
	return EXIT_SUCCESS;
}

//true => succesful exit
static bool step_1 (machine_t* machine) {
	command_t cmd = {};
	if (red_update) {
		red_update = false;
		mark_byte (red_pos, machine, GREY);
	}
	error_t err = step (machine);
	const word rc = machine->registers[RC];
	get_command (&cmd, machine, rc);
	mark_byte (rc, machine, WHITE);
	if (sizes[cmd._type] == 2) {
		mark_byte (rc + 1, machine, WHITE);
	}
	print_registers (machine);
	wrefresh (registers);
	wrefresh (program);
	if (err != E_OK) {
		const unsigned _line = 1 + FULL + 1 + 1;
		wmove (text, _line, 0);
		wclrtoeol (text);
		switch (err) {
		case E_OK:
			break;
		case E_DIVZERO:
			mvwprintw (text, _line, 0, "Error: DIVBYZERO");
			break;
		case E_UNKNOWN:
			mvwprintw (text, _line, 0, "Error: unknown opcode 0xf");
			break;
		case E_LOOP:
			mvwprintw (text, _line, 0, "Correct end! Enter quit.");
			break;
		}
		wrefresh (text);
	}
	disassembling (machine);
	return err == E_OK;
}

static void running (machine_t* machine) {
	while (true) {
		if (!step_1 (machine)) {
			return;
		}
		usleep (DELTA);
	}
}

int run_emulator (machine_t* machine) {
	int exit_code = init (machine);
	if (exit_code == EXIT_FAILURE) {
		goto END;
	}
	move (PROG_HEIGHT + TEXT_HEIGHT + 1, 2);
	command_t cmd = {};
	get_command (&cmd, machine, 0);
	mark_byte (0, machine, WHITE);
	if (sizes[cmd._type] == 2) {
		mark_byte (1, machine, WHITE);
	}
	disassembling (machine);
	//noecho ();
	//timeout (-1);
	//keypad (stdscr, TRUE);
	//meta (stdscr, FALSE);
	//cbreak ();
	while (true) {
		if (!read_line ()) {
			exit_code = EXIT_FAILURE;
			break;
		}
		if (strcmp (line, "quit") == 0) {
			break;
		}
		wmove (text, 1 + FULL + 1 + 1, 0);
		wclrtoeol (text);
		mvwprintw (text, 1 + FULL + 1 + 1, 0, "Got command \"%s\"", line);
		wrefresh (text);
		if (strcmp (line, "step") == 0) {
			step_1 (machine);
		} else if (strcmp (line, "start") == 0) {
			running (machine);
		}

		wmove (input, 1, 2);
		wclrtoeol (input);
		move (PROG_HEIGHT + TEXT_HEIGHT + 1, 2);
		wrefresh (input);
	}
END:;	const int _old = errno;
	clear_resources ();
	PRINT("Exit code %d\n", exit_code);
	if (exit_code == EXIT_FAILURE) {
		printf ("String:");
		for (char* c = line; *c; ++c) {
			printf (" %02x", *c);
		}
		printf ("\n");
		PANIC("Errno: %s\n", strerror (_old));
	}
	return exit_code;
}

void clear_resources () {
#define FREE(window) \
	if (window != NULL) { \
		delwin (window); \
		window = NULL; \
	}
	FREE(registers);
	FREE(program);
	FREE(text);
	FREE(input);
#undef FREE
	endwin ();
	cleanup_random ();
}
