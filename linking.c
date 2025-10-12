#include "common.h"

int linker (parsed_t* prog, word offset) {
	const unsigned cmd_count = prog->cmd_real, cmd_alloc = prog->cmd_count;
	const word m_count = prog->m_count;

	command_ext_t* cmds  = prog->commands;
	mark_info_t*   marks = prog->marks;
	for (word i = 0; i < m_count; ++i) {
		if (marks[i].cmd_id == cmd_alloc) {
			PANIC("macro \"%.*s\" used without declaration\n", marks[i].mark_len, marks[i].name);
			return EXIT_FAILURE;
		}
	}
	word* addresses = alloc(word, cmd_count);
	if (addresses == NULL) {
		PANIC ("couldn't allocate %lu bytes (%u objects)\n",
			size_calc(word, cmd_count), cmd_count);
		return EXIT_FAILURE;
	}
	PRINT ("Allocated %lu bytes (%u objects)\n",
			size_calc(word, cmd_count), cmd_count);
	
	memset (addresses, 0, size_calc(word, cmd_count));
	int exit_code = EXIT_SUCCESS;
	for (unsigned i = 0; i < cmd_count; ++i) {
		const word size = sizes[cmds[i]._._type];
		if ((word)(offset + size) < offset) {
			PANIC ("ERROR: program too large\n");
			exit_code = EXIT_FAILURE;
			goto END;
		}
		addresses[i] = offset;
		offset += size;
	}
	for (word i = 0; i < m_count; ++i) {
		marks[i].addr = addresses[marks[i].cmd_id];
		if (marks[i].mark_len == 1 && marks[i].name[0] == NEXT) {
			++marks[i].addr;
		}
	}
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
			PANIC ("ERROR: mark is used as parameter illegally!\nCommand #%u\n", i);
			exit_code = EXIT_FAILURE;
			goto END;
		}
	}
	PRINT ("Last address: %u\n", addresses[cmd_count - 1]);
END:
	free (addresses);
	addresses = NULL;
	return exit_code;
}
