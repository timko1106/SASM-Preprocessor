#include "common.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

typedef struct {
	word buffer[256];
} code_t;

// assembler:
// Каждая команда имеет 1 байт вида (от старшего к младшему)
// type(4) dest (2) src (2)
//Дополнительный параметр: 1 байт.
void assembler (parsed_t* prog, int fd) {
	code_t code = {};//memzero.
	memset (code.buffer, 0, sizeof (code.buffer));
	const unsigned cmd_count = prog->cmd_count;
	const command_ext_t* cmd = prog->commands;
	uint16_t address = 0;
	for (unsigned i = 0; i < cmd_count; ++i) {
		const command_t _cmd = cmd[i]._;
		const cmd_t _type = _cmd._type;
		//Comment.
		if (_type == C_NONE) {
			continue;
		}
		word byte1 = 0;
		//High 4 bits
		byte1 = (_type << 4);
		//Low 4 bits
		byte1 |= (_cmd.dst << 2);
		byte1 |= _cmd.src;

		code.buffer[address++] = byte1;
		if (sizes[_type] == 2) {
			code.buffer[address++] = _cmd.param;
		}
	}
	//Полную программу не кидаем (для простоты дизассемблера)
	if (address) {
		write (fd, code.buffer, address);
	}
}

//Конвейер parse + link + assembler
int assembly (state_t* state, int fd) {
	parsed_t prog = {};

	int exit_code = EXIT_SUCCESS;
	if (parse (state, &prog, false) == EXIT_FAILURE) {
		exit_code = EXIT_FAILURE;
		goto END;
	}
	if (linker (&prog, 0) == EXIT_FAILURE) {
		exit_code = EXIT_FAILURE;
		goto END;
	}
	assembler (&prog, fd);
	PRINT ("Succesfully assembled!\n");
END:	data_parsed_free (&prog);
	return exit_code;
}

int main (int argc, const char** argv) {
	if (argc != 3) {
		PANIC ("Need 2 arguments:\n1) Input file compatible with SASM\n2) Output binary file (program)\n");
		return EXIT_FAILURE;
	}
	const int fd1 = open (argv[1], O_RDONLY);
	if (fd1 == -1) {
		PANIC ("Couldn't open input file \"%s\": %s\n", argv[1], strerror (errno));
		return EXIT_FAILURE;
	}
	const int fd2 = open (argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0640);
	if (fd2 == -1) {
		PANIC ("Couldn't open output file \"%s\": %s\n", argv[2], strerror (errno));
		close (fd2);
		return EXIT_FAILURE;
	}
	//Прочитать код
	state_t text = {};
	text.length = read (fd1, text.buffer, sizeof (text.buffer) - 1);
	text.buffer[text.length] = 0;
	//Assemble.
	const int exit_code = assembly (&text, fd2);
	close (fd2);//Не знаю.
	close (fd1);//Традиция (это про обратный порядок закрытия).
	return exit_code;
}
