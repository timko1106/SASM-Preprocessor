#include "preprocessor.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

int preprocess (state_t* state, int fd_out) {
	/*const char start[] = "R0" RSP " <- 0xFF\nRC <- @_init\n";
	write (fd_out, start, sizeof (start) - 1);*/

	state_t next = {};
	next.buffer[0] = 0;
	next.length = 0;
	
	if (stage_1 (state, &next)) {
		return EXIT_FAILURE;
	}
	printf ("%.*s", (int)next.length, next.buffer);
	write (fd_out, next.buffer, next.length);
	return EXIT_SUCCESS;
	/*Future stages #2 and #3.

	word count = 0;
	word *loads = NULL, *stores = NULL;
	command_ext_t* commands = NULL;
	char** marks = NULL;
	int exit_code = EXIT_SUCCESS;
	if (stage_2 (&next, &commands, &marks, &loads, &stores, &count) == EXIT_FAILURE) {
		goto cleanup;
	}
cleanup:
	if (marks != NULL) {
		free (marks);
		marks = NULL;
	}
	if (commands != NULL) {
		free (commands);
		commands = NULL;
	}
	if (loads != NULL) {
		free (loads);
		loads = NULL;
	}
	if (stores != NULL) {
		free (stores);
		stores = NULL;
	}
	return exit_code;*/
}

int main (int argc, const char** argv) {
	if (argc == 0 || argc != 3) {
		printf ("Need 2 arguments:\n1) Name of input file\n2) Name of output file\n");
		return EXIT_FAILURE;
	}
	int fd1 = open (argv[1], O_RDONLY);
	if (fd1 == -1) {
		printf ("Couldn't open input file: %s\n", strerror (errno));
		return EXIT_FAILURE;
	}
	int fd2 = open (argv[2], O_WRONLY | O_CREAT, 0640);
	if (fd2 == -1) {
		printf ("Couldn't open output file: %s\n", strerror (errno));
		close (fd1);
		return EXIT_FAILURE;
	}
	state_t state = {};
	state.buffer[0] = 0;

	state.length = read (fd1, state.buffer, LIMIT - 1);
	state.buffer[state.length] = 0;

	int exit_code = preprocess (&state, fd2);

	close (fd2);
	close (fd1);
	return exit_code;
}
