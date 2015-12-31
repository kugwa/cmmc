#ifndef CCMMC_HEADER_STATE_H
#define CCMMC_HEADER_STATE_H

#include "ast.h"
#include "symbol-table.h"
#include "register.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// All states of the compiler instance
typedef struct CcmmcState_struct {
    CcmmcAst *ast;
    CcmmcSymbolTable *table;
    size_t line_number;
    FILE *asm_output;
    CcmmcRegPool *reg_pool;
} CcmmcState;

void             ccmmc_state_init                   (CcmmcState *state);
void             ccmmc_state_fini                   (CcmmcState *state);

#endif
// vim: set sw=4 ts=4 sts=4 et:
