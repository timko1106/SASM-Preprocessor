#include "emulator.h"
#include <fcntl.h>
#include <unistd.h>

static int fd = -1;

static word entropy[16] = {};
static unsigned pos = sizeof(entropy);

bool init_random (void) {
	fd = open ("/dev/urandom", O_RDONLY);
	if (fd < 0) {
		fd = -1;
		return false;
	}
	return true;
}

static word get_random (void) {
	if (pos == sizeof(entropy)) {
		read (fd, &entropy, sizeof(entropy));
		pos = 0;
	}
	return entropy[pos++];
}

void cleanup_random (void) {
	if (fd != -1) {
		close (fd);
		fd = -1;
	}
}


static void set_flags (machine_t* state, uint16_t n) {
	word rf = 0;
	rf |= ((n & 0xFF ) ? 0 : 1);//Zero
	rf |= ((0x80 & n ) ? 2 : 0);//Sign
	rf |= ((n & 0x100) ? 4 : 0);//Carry
	state->registers[RF] = rf;
}

int get_command (command_t* cmd, const machine_t* state, word rc) {
	const word major = state->buffer[rc];
	const cmd_t _type = (cmd_t)(major >> 4);
	cmd->_type = _type;
	if (_type == C_NONE) {
		return EXIT_FAILURE;
	}
	const reg_t dst = (major >> 2) & 3,
		src = major & 3;
	const word param = (sizes[_type] == 2 ? state->buffer[rc + 1] : 0);

	cmd->_type = _type;
	cmd->dst = dst;
	cmd->src = src;
	cmd->param = param;
	return EXIT_SUCCESS;
}

error_t step (machine_t* machine) {
	command_t cmd = {};
	if (get_command (&cmd, machine, machine->registers[RC]) == EXIT_FAILURE) {
		return E_UNKNOWN;
	}
	const reg_t src = cmd.src, dst = cmd.dst;
	const word param = cmd.param;

	const uint16_t _dst = machine->registers[dst], _src = machine->registers[src];

	bool jump = false;
	const word old_rc = machine->registers[RC];
	switch (cmd._type) {
	case C_MOV:
		machine->registers[dst] = _src;;
		break;
	case C_LOAD:
		machine->registers[dst] = machine->buffer[param];
		break;
	case C_STORE:
		machine->buffer[param] = _src;
		mark_byte (param, machine, RED);
		break;
	case C_MOV_V:
		machine->registers[dst] = param;
		break;
	case C_ADD:
	case C_SUB:
	case C_MUL:
	case C_DIV:;
		{
		if (cmd._type == C_DIV && _src == 0) {
			return E_DIVZERO;
		}
		uint16_t res = 0;
		if (cmd._type == C_ADD) {
			res = _dst + _src;
		} else if (cmd._type == C_SUB) {
			res = _dst + 1 + ((~_src) & 0xFF);
		} else if (cmd._type == C_MUL) {
			res = _dst * _src;
		} else if (cmd._type == C_DIV) {
			res = _dst / _src;
		}
		set_flags (machine, res);
		machine->registers[dst] = (res & 0xFF);
		}
		break;
	case C_CMP:
		{
		uint16_t res = machine->registers[dst];
		res += 1 + ((~machine->registers[src]) & 0xFF);
		set_flags (machine, res);
		}
		break;
	case C_JMP_C:
	case C_JMP_S:
	case C_JMP_Z:
	case C_JMP:
		{
		const word rf = machine->registers[RF];
		if (cmd._type == C_JMP) {
			jump = true;
		} else if (cmd._type == C_JMP_C) {
			jump = (rf & 4);
		} else if (cmd._type == C_JMP_S) {
			jump = (rf & 2);
		} else {
			jump = (rf & 1);
		}
		if (jump) {
			machine->registers[RC] = param;
		}
		}
		break;
	case C_RAND:
		machine->registers[dst] = get_random ();
		break;
	case C_INC:
		{
		uint16_t val = _src;
		++val;
		set_flags (machine, val);
		machine->registers[dst] = val;
		}
		break;
	case C_NONE:
		break;
        }
	mark_byte (old_rc, machine, GREY);
	if (sizes[cmd._type] == 2) {
		mark_byte (old_rc + 1, machine, GREY);
	}
	if (jump && machine->registers[RC] == old_rc) {
		return E_LOOP;
	}
	if (!jump) {
		machine->registers[RC] += sizes[cmd._type];
	}
	return E_OK;
}
