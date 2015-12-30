#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "code-generation.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void generate_global_variable(CcmmcAst *global, CcmmcState *state)
{
}

static void generate_function(CcmmcAst *funcion, CcmmcState *state)
{
}

static void generate_program(CcmmcState *state)
{
    for (CcmmcAst *global_decl = state->ast->child; global_decl != NULL;
         global_decl = global_decl->right_sibling) {
        switch (global_decl->value_decl.kind) {
            case CCMMC_KIND_DECL_VARIABLE:
                generate_global_variable(global_decl, state);
                break;
            case CCMMC_KIND_DECL_FUNCTION:
                generate_function(global_decl, state);
                break;
            case CCMMC_KIND_DECL_FUNCTION_PARAMETER:
            case CCMMC_KIND_DECL_TYPE:
            default:
                assert(false);
        }
    }
}

void ccmmc_code_generation(CcmmcState *state)
{
    generate_program(state);
}
