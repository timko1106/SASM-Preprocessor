#include "preprocessor.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

int preprocess (state_t* state, int fd_1, int fd_2) {
	/*const char start[] = "R0" RSP " <- 0xFF\nRC <- @_init\n";
	write (fd_out, start, sizeof (start) - 1);*/

	state_t next = {};
	next.buffer[0] = 0;
	next.length = 0;
	
	if (stage_1 (state, &next)) {
		return EXIT_FAILURE;
	}
	write (fd_1, next.buffer, next.length);
	state_t final = {};
	final.buffer[0] = 0;
	final.length = 0;
	//return EXIT_SUCCESS;
	//Future stages #2 and #3.

	parsed_ext_t parsed = {};

	int exit_code = EXIT_SUCCESS;
	if (stage_2 (&next, &parsed) == EXIT_FAILURE) {
		exit_code = EXIT_FAILURE;
		goto cleanup;
	}
	exit_code = stage_3 (&final, &parsed);
	if (exit_code == EXIT_SUCCESS) {
		write (fd_2, final.buffer, final.length);
		printf ("Succesfully preprocessed!\n");
	}
cleanup:
	data_parsed_ext_free (&parsed);
	return exit_code;
}

int main (int argc, const char** argv) {
	if (argc == 0 || argc != 1 +3) {
		printf ("Need 3 arguments:\n"
			"1) Name of input file\n"
			"2) Name of output file with expanded macroses\n"
			"3) Name of output file\n");
		return EXIT_FAILURE;
	}
	int exit_code = EXIT_SUCCESS;
	int fd1 = -1, fd2 = -1, fd3 = -1;
	fd1 = open (argv[1], O_RDONLY);
	if (fd1 == -1) {
		printf ("Couldn't open input file: %s\n", strerror (errno));
		return EXIT_FAILURE;
	}
	fd2 = open (argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0640);
	if (fd2 == -1) {
		printf ("Couldn't open output file 1: %s\n", strerror (errno));
		exit_code = EXIT_FAILURE;
		goto END;
	}
	fd3 = open (argv[3], O_WRONLY | O_CREAT | O_TRUNC, 0640);
	if (fd3 == -1) {
		printf ("Couldn't open output file 2: %s\n", strerror (errno));
		exit_code = EXIT_FAILURE;
		goto END;
	}
	{
	state_t state = {};
	state.buffer[0] = 0;

	state.length = read (fd1, state.buffer, LIMIT - 1);
	state.buffer[state.length] = 0;

	exit_code = preprocess (&state, fd2, fd3);
	}
END:
	if (fd3 != -1) {
		close (fd3);
	}
	if (fd2 != -1) {
		close (fd2);
	}
	if (fd1 != -1) {
		close (fd1);
	}
	return exit_code;
}
