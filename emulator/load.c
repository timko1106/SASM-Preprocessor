#include "emulator.h"
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

int main (int argc, const char** argv) {
	if (argc != 2) {
		PANIC("Need 1 argument:\n"
			"1) input compiled file\n");
		return EXIT_FAILURE;
	}
	int fd = open (argv[1], O_RDONLY);
	if (fd == -1) {
		PANIC ("Can't open file \"%s\": %s\n", argv[1], strerror (errno));
		return EXIT_FAILURE;
	}
	int exit_code = EXIT_SUCCESS;
	machine_t machine = {};
	read (fd, machine.buffer, sizeof (machine.buffer));
	close (fd);
	exit_code = run_emulator (&machine);
	return exit_code;
}
