#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "state.h"


void ccmmc_state_init (CcmmcState *state)
{
	state->ast = NULL;
	state->line_number = 1;
	state->any_error = false;
}

void ccmmc_state_fini (CcmmcState *state)
{
	if (state->ast != NULL) {
		// TODO: Free the AST
	}
}
