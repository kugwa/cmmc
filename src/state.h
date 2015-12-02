#ifndef CCMMC_HEADER_STATE_H
#endif

#include "ast.h"
#include "symbol-table.h"

#include <stdbool.h>
#include <stddef.h>

// All states of the compiler instance
typedef struct CcmmcState_struct {
	CcmmcAst *ast;
	size_t line_number;
	bool any_error;
} CcmmcState;

void             ccmmc_state_init                   (CcmmcState *state);
void             ccmmc_state_fini                   (CcmmcState *state);

// vim: set sw=4 ts=4 sts=4 et:
#define CCMMC_HEADER_STATE_H
