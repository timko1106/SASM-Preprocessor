#include "preprocessor.h"


int stage_2 (state_t* state, parsed_ext_t* prog) {
	return parse (state, (parsed_t*)prog, true);
}
