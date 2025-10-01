#include "preprocessor.h"
#include <ctype.h>
#include <stdbool.h>

typedef struct {
	const char* name, *definition;// Stored in modified source code
	word name_len, arg_count, def_len;
} macro_info;

static bool is_valid (char x) {
	return isalpha (x) || isdigit (x) || x == '_';
}

static state_t macroses = {};//macro expansions.
static state_t temps = {};//temporary macro arguments' expansion (in direct usage) and no-args macro definitions (should be copied to macroses).

#define MAX_MACRO_ARGS 10

static int macro_expand (const char** src, unsigned length, macro_info* infos, unsigned detected, char** dst, unsigned fast_check, bool allow_recurse) {
	const char* ptr = *src;
	char* ptr2 = *dst;
	for (unsigned i = 0; i < length;) {
		const char c = *ptr;
		if (c != MACRO_USE) {
			++ptr;
			++i;
			*ptr2++ = c;
			continue;
		}
		++i;
		++ptr;
		unsigned name_len = 0;
		while (i < length && is_valid (ptr[name_len])) {
			++name_len;
			++i;
		}
		const char* name = ptr;
		ptr += name_len;
		word pos = -1;
		bool found = false;
#define VALIDATE if (name_len == infos[j].name_len && memcmp (name, infos[j].name, name_len) == 0) { \
				found = true; \
				pos = j; \
				break; \
			}
		for (unsigned j = fast_check; j < detected; ++j) {
			VALIDATE
		}
		if (!found) {
			for (unsigned j = 0; j < fast_check; ++j) {
				VALIDATE
			}
		}
#undef VALIDATE
		if (!found) {
			printf ("ERROR: Couldn't find macro %.*s\n", name_len, name);
			return EXIT_FAILURE;
		}
		macro_info* info = &(infos[pos]);
		if (ptr[0] == ARG_OPEN) {
			if (info->arg_count == 0) {
				printf ("ERROR: macro %.*s doesn't use arguments\n", name_len, name);
				return EXIT_FAILURE;
			}
			if (allow_recurse) {
				word locs[MAX_MACRO_ARGS + 1] = {};//From '('.
				locs[0] = 0;
				unsigned nargs = 1, j = 1;
				for (++i; i < length; ++i, ++j) {
					const char c = ptr[j];
					if (c == ARG_SEP) {
						locs[nargs++] = j;
						if (nargs > infos->arg_count) {
							printf ("ERROR: macro %.*s requires %d arguments, provided %d\n", 
									info->name_len, info->name, info->arg_count, nargs);
							return EXIT_FAILURE;
						}
						continue;
					}
					if (c == ARG_CLOSE) {
						locs[nargs] = j;
						break;
					}
					//Do nothing.
				}
				if (ptr[j] != ARG_CLOSE) {
					printf ("ERROR: macro %.*s should end by %c\n", info->name_len, info->name, ARG_CLOSE);
					return EXIT_FAILURE;
				}
				if (nargs != info->arg_count) {
					printf ("ERROR: macro %.*s requires %d arguments, provided %d\n", 
							info->name_len, info->name, info->arg_count, nargs);
					return EXIT_FAILURE;
				}
				printf ("Macro %.*s(%.*s)\n", info->name_len, info->name, j - 1, ptr + 1);
				//Make temporary macroses:
				for (unsigned k = 0; k < nargs; ++k) {
					static char all_names[] = "0123456789";
					infos[detected + k].arg_count = 0;
					infos[detected + k].definition = ptr + locs[k] + 1;
					infos[detected + k].def_len = locs[k + 1] - (locs[k] + 1);
					infos[detected + k].name = &(all_names[k]);
					infos[detected + k].name_len = 1;
				}
				const char* _definition = info->definition;
				if (macro_expand (&_definition, info->def_len, infos, detected + nargs, &ptr2, detected, false) == EXIT_FAILURE) {
					return EXIT_FAILURE;
				}
				ptr += j;

				++ptr;//')'
				++i;
				continue;
			}
			printf ("ERROR: Recursive macroses are not supported (expanding %.*s)\n", name_len, name);
			return EXIT_FAILURE;
		}
		memcpy_s (ptr2, info->definition, info->def_len);
		ptr2 += info->def_len;
	}
	*src = ptr;
	*dst = ptr2;
	return EXIT_SUCCESS;
}

static int macro_define (const char** _ptr, macro_info* infos, unsigned detected) {
	int exit_code = EXIT_SUCCESS;
	const char* ptr = (char*)*_ptr;
	char* ptr2 = macroses.buffer + macroses.length;
	const char* const ptr2_old = ptr2;
	/*
	Comments FORBIDDEN!
	#macro_name(arg_name) (SPACE)line_1 \
	line_2 \
	line_3 \
	...
	line_(n-1) \
	line_n

	OR:
	#macro_name (SPACE)line_1 \
	line_2 \
	...
	line_(n-1) \
	line_n
	*/
	++ptr;//# пропускаем.
	const char* _old = ptr;
	while (*ptr != ARG_OPEN && *ptr != ' ' && *ptr != 0) {
		if (!is_valid (*ptr)) {
			printf ("ERROR: macro can include only digits, letters and '_' (see %.*s)\n", (int)(ptr - _old + 1), _old);
			return EXIT_FAILURE;
		}
		++ptr;
	}
	unsigned name_len = ptr - _old;

	macro_info _macro = {};
	macro_info* info = &_macro;
	info->name = _old;
	info->name_len = name_len;

	if (*ptr == ' ') {
		char* ptr_tmp = temps.buffer + temps.length;
		char* ptr_tmp_old = ptr_tmp;
		++ptr;
		info->arg_count = 0;
		info->definition = ptr2;
		unsigned shift = 0;
		bool is_ok = true;
		while (is_ok) {
			is_ok = false;
			char c = ptr[shift];
			while (c != MACRO_CONTINUE && c != LF && c != 0) {
				*ptr_tmp++ = c;
				//*ptr2++ = c;
				//ptr[def_len++] = c;
				c = ptr[++shift];
			}
			if (c == MACRO_CONTINUE) {
				is_ok = true;
				while (c != LF && c != 0) {
					c = ptr[++shift];
					*ptr_tmp++ = c;
					//*ptr2++ = c;
					//ptr[def_len++] = c;
				}
			}
			if (c == LF) {
				++shift;
			}
		}
		const unsigned length = ptr_tmp - ptr_tmp_old;
		temps.length += length;
		if (macro_expand ((const char**)&ptr_tmp_old, length, infos, detected, &ptr2, 0, true) == EXIT_FAILURE) {
			exit_code = EXIT_FAILURE;
			goto END;
		}
		temps.length -= length;
		info->def_len = ptr2 - ptr2_old;
		ptr += shift;
	} else if (*ptr == '(') {
		word locs[MAX_MACRO_ARGS + 1] = {};//From '('.
		locs[0] = 0;
		unsigned nargs = 1, j = 1;
		for (; ptr[j] != LF && ptr[j] != 0; ++j) {
			const char c = ptr[j];
			if (c == ARG_SEP) {
				locs[nargs] = j;
				printf ("Macro %.*s argument \"%.*s\"\n", info->name_len, info->name, 
					locs[nargs] - (locs[nargs - 1] + 1), ptr + locs[nargs - 1] + 1);
				++nargs;
				if (nargs > MAX_MACRO_ARGS) {
					printf ("ERROR: to many arguments (at most %u)\n", MAX_MACRO_ARGS);
					exit_code = EXIT_FAILURE;
					goto END;
				}
				continue;
			}
			if (c == ARG_CLOSE) {
				locs[nargs] = j;
				printf ("Macro %.*s argument %.*s\n", info->name_len, info->name, 
					locs[nargs] - (locs[nargs - 1] + 1), ptr + locs[nargs - 1] + 1);
				break;
			}
			if (!is_valid (c)) {
				printf ("Invalid character in argumentf of macro %.*s\n", info->name_len, info->name);
				exit_code = EXIT_FAILURE;
				goto END;
			}
			//Do nothing.
		}
		if (ptr[j] != ARG_CLOSE) {
			printf ("ERROR: macro %.*s should end by %c\n", info->name_len, info->name, ARG_CLOSE);
			exit_code = EXIT_FAILURE;
			goto END;
		}
		info->arg_count = nargs;
		//Make temporary macroses:
		for (unsigned k = 0; k < nargs; ++k) {
			static const char all_names[] = "%0%1%2%3%4%5%6%7%8%9";
			infos[detected + k].arg_count = 0;
			infos[detected + k].name = ptr + locs[k] + 1;
			infos[detected + k].name_len = locs[k + 1] - (locs[k] + 1);
			infos[detected + k].definition = &(all_names[2 * k]);
			infos[detected + k].def_len = 2;
			printf ("Add argument: \"%.*s\", definition: \"%.*s\"\n", 
				infos[detected + k].name_len, infos[detected + k].name,
				infos[detected + k].def_len, infos[detected + k].definition);
		}

		ptr += j + 1;
		info->definition = ptr2;

		unsigned shift = 0;
		bool is_ok = true;
		while (is_ok) {
			is_ok = false;
			char c = ptr[shift];
			while (c != MACRO_CONTINUE && c != LF && c != 0) {
				if (c != MACRO_USE) {
					*ptr2++ = c;
					c = ptr[++shift];
					continue;
				}
				const char* ptr_real = ptr + shift;
				const char* ptr_ = ptr_real + 1;
				while (is_valid (*ptr_)) ++ptr_;
				if (*ptr_ == ARG_OPEN) {
					while (*ptr_ != ARG_CLOSE && *ptr_ != LF && *ptr_ != 0) ++ptr_;
					if (*ptr_ != ARG_CLOSE) {
						printf ("ERROR: incorrect argument used in macro %.*s:\nExpansion of:\n\"%.*s\"\n",
								name_len, _old, (int)(ptr_ - ptr_real + 1), ptr_real);
						exit_code = EXIT_FAILURE;
						goto END;
					}
				}
				shift += ptr_ - ptr_real;
				if (macro_expand (&ptr_real, ptr_ - ptr_real, infos, detected + nargs, &ptr2, detected, true) == EXIT_FAILURE) {
					exit_code = EXIT_FAILURE;
					goto END;
				}
				c = ptr[shift];
			}
			if (c == MACRO_CONTINUE) {
				is_ok = true;
				while (c != LF && c != 0) {
					c = ptr[++shift];
					*ptr2++ = c;
				}
			}
			if (c == LF) {
				++shift;
			}
		}
		info->def_len = ptr2 - ptr2_old;
		ptr += shift;
	} else {
		printf ("ERROR: illegal macro declaration, should be \"%.*s \"...\" or \"%.*s(...)\"",
				info->name_len, info->name, info->name_len, info->name);
		exit_code = EXIT_FAILURE;
		goto END;
	}
	printf ("Declared macros %.*s:\n\"%.*s\"\n\n\n\n\n\n", info->name_len, info->name, info->def_len, info->definition);
	macroses.length += info->def_len;
END:	*_ptr = ptr;
	infos[detected] = _macro;
	return exit_code;
}

int stage_1 (state_t* src, state_t* dest) {
	const char rsp_macro[] = rsp;//#rsp R03
	const char rsp_def[] = "R0" RSP;

	unsigned count = 1 + (src->buffer[0] == MACRO_DEF);

	char prev = src->buffer[0], curr;
	for (const char *start = src->buffer + 1; *start; ++start) {
		curr = *start;
		count += ((curr == MACRO_DEF) && (prev == LF));

		prev = curr;
	}
	const unsigned BLOCKS = count + 2 * MAX_MACRO_ARGS;
	printf ("To allocate: %lu bytes (%u blocks)\n", size_calc (macro_info, BLOCKS), BLOCKS);
	macro_info* infos = alloc (macro_info, count + 2 * MAX_MACRO_ARGS);
	if (infos == NULL) {
		printf ("ERROR: can't allocate %lu bytes\n", size_calc (macro_info, BLOCKS));
		return EXIT_FAILURE;
	}
	infos[0].arg_count = 0;//rsp register.
	infos[0].name = rsp_macro;
	infos[0].definition = rsp_def;
	infos[0].name_len = sizeof (rsp_macro) - 1;
	infos[0].def_len = sizeof (rsp_def) - 1;

	unsigned detected = 1;
	int exit_code = EXIT_SUCCESS;
	for (const char* ptr = src->buffer; *ptr;) {
		if (*ptr == MACRO_DEF) {
			if (macro_define (&ptr, infos, detected) != EXIT_SUCCESS) {
				exit_code = EXIT_FAILURE;
				break;
			}
			++detected;
			continue;
		}
		while (true) {//For all characters of the line
			const char x = *ptr;
			if (x == LF) {
				dest->buffer[dest->length++] = LF;
				++ptr;
				break;
			}
			if (x == 0) {
				goto END;
			}
			if (x == MACRO_USE) {
				++ptr;//first byte of macro name
				unsigned len = 0;
				while (is_valid (*(ptr + len))) {
					++len;
				}
				//Search for macro:
				word macro_idx = -1;
				bool found = false;
				//ptr[0...len-1] - macro name
				for (word i = 0; i < detected; ++i) {
					if (infos[i].name_len == len && memcmp (infos[i].name, ptr, len) == 0) {
						macro_idx = i;
						found = true;
						break;
					}
				}
				if (!found) {
					printf ("ERROR: couldn't find macro %.*s\n", len, ptr);
					exit_code = EXIT_FAILURE;
					goto END;
				}
				const macro_info* macro = &(infos[macro_idx]);
				if ((ptr[len] == ARG_OPEN) != (macro->arg_count != 0)) {
					printf ("ERROR: macro %.*s should have %d arguments, but gets %d\n", 
							len, ptr, macro->arg_count, ptr[len] == ARG_OPEN);
					exit_code = EXIT_FAILURE;
					goto END;
				}
				if (ptr[len] == ARG_OPEN) {
					unsigned full = len + 2;//1'%' +len+1 '('
					--ptr;//byte % before name of macro

					char* dst_step1 = temps.buffer + temps.length;
					char* dst_step1_old = dst_step1;
					memcpy_s(dst_step1, ptr, full);
					dst_step1 += full;

					while (ptr[full] != ARG_CLOSE && ptr[full] != LF && ptr[full] != 0) {
						++full;
					}
					if (ptr[full] == ARG_CLOSE) {
						++full;
					} else {
						printf ("ERROR: wrong macro usage \"%.*s\"\n", full + 1, ptr);
						exit_code = EXIT_FAILURE;
						goto END;
					}
					{
					const char* _ptrk = ptr + 1 + macro->name_len + 1;
					// - ((%) + name_len + ('(' )) - ')'
					if (macro_expand (&_ptrk, full - (1 + macro->name_len + 1) - 1, infos, detected, &dst_step1, 0, true) == EXIT_FAILURE) {
						exit_code = EXIT_FAILURE;
						goto END;
					}
					}

					*dst_step1++ = ARG_CLOSE;
					const unsigned length = dst_step1 - dst_step1_old;
					temps.length += length;

					char* _new = dest->buffer + dest->length;
					char* _old = _new;
					if (macro_expand ((const char**)&dst_step1_old, length, infos, detected, &_new, macro_idx, true) == EXIT_FAILURE) {
						exit_code = EXIT_FAILURE;
						goto END;
					}
					temps.length -= length;
					ptr += full;
					dest->length += (_new - _old);
				} else {
					printf ("Macro %.*s\n", macro->name_len, macro->name);
					memcpy_s (dest->buffer + dest->length, macro->definition, macro->def_len);
					dest->length += macro->def_len;

					ptr += len;
				}
				continue;
			}
			dest->buffer[dest->length++] = *(ptr++);
		}
	}
END:;
	free (infos);
	dest->buffer[dest->length] = 0;
	return exit_code;
}
