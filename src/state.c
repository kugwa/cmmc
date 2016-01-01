#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "state.h"


void ccmmc_state_init (CcmmcState *state)
{
    state->ast = NULL;
    state->table = NULL;
    state->line_number = 1;
    state->asm_output = NULL;
    state->reg_pool = NULL;
    state->label_number = 0;
}

void ccmmc_state_fini (CcmmcState *state)
{
    if (state->ast != NULL) {
        // TODO: Free the AST
    }
    if (state->table != NULL) {
        // TODO: Free the symbol table
    }
    if (state->asm_output != NULL) {
        fclose(state->asm_output);
    }
    if (state->reg_pool != NULL) {
        ccmmc_register_fini(state->reg_pool);
    }
}

// vim: set sw=4 ts=4 sts=4 et:
