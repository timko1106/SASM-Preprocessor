#include "common.h"

//Линковщик: всем меткам сопоставляет их адреса.
int linker (parsed_t* prog, word offset) {
	const unsigned cmd_count = prog->cmd_real, cmd_alloc = prog->cmd_count;
	const word m_count = prog->m_count;

	command_ext_t* cmds  = prog->commands;
	mark_info_t*   marks = prog->marks;
	//1: check for marks without definitions
	for (word i = 0; i < m_count; ++i) {
		if (marks[i].cmd_id == cmd_alloc) {
			PANIC("Macro \"%.*s\" used without declaration\n", marks[i].mark_len, marks[i].name);
			return EXIT_FAILURE;
		}
	}
	word* addresses = alloc(word, cmd_count);
	if (addresses == NULL) {
		PANIC ("Couldn't allocate %lu bytes (%u objects)\n",
			size_calc(word, cmd_count), cmd_count);
		return EXIT_FAILURE;
	}
	PRINT ("Allocated %lu bytes (%u objects)\n",
			size_calc(word, cmd_count), cmd_count);
	//2: set addresses for marks.
	memset (addresses, 0, size_calc(word, cmd_count));
	int exit_code = EXIT_SUCCESS;
	uint16_t length = offset;
	for (unsigned i = 0; i < cmd_count; ++i) {
		const word size = sizes[cmds[i]._._type];
		if (length + size > 0x100) {
			PANIC ("ERROR: program too large\n");
			exit_code = EXIT_FAILURE;
			goto END;
		}
		addresses[i] = offset;
		offset += size;
		length += size;
	}
	for (word i = 0; i < m_count; ++i) {
		marks[i].addr = addresses[marks[i].cmd_id];
		if (marks[i].mark_len == 1 && marks[i].name[0] == NEXT) {
			++marks[i].addr;
		}
	}
	//3: process marks
	for (unsigned i = 0; i < cmd_count; ++i) {
		const word mark = cmds[i].mark_id;
		if (mark == m_count) {
			continue;
		}
		switch (cmds[i]._._type) {
		case C_LOAD:
		case C_STORE:
		case C_MOV_V:
			cmds[i].mark_id = m_count;
			cmds[i]._.param = marks[mark].addr;
			break;
		case C_JMP:
		case C_JMP_C:
		case C_JMP_S:
		case C_JMP_Z:
			if (marks[mark].mark_len == 1 && 
				(*marks[mark].name == JUMP || *marks[mark].name == ADDR || *marks[mark].name == NEXT)) {
				cmds[i].mark_id = m_count;
			}
			cmds[i]._.param = marks[mark].addr;
			break;
		default:
			PANIC ("Mark is used as parameter illegally!\nCommand #%u\n", i);
			exit_code = EXIT_FAILURE;
			goto END;
		}
	}
	PRINT ("Size: %hu\n", length);
END:
	free (addresses);
	addresses = NULL;
	return exit_code;
}
