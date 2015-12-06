#ifndef CCMMC_HEADER_DRAW_H
#define CCMMC_HEADER_DRAW_H

#include "ast.h"
#include "symbol-table.h"

#include <stdio.h>

void             ccmmc_draw_ast                     (FILE *fp,
                                                     const char *name,
                                                     CcmmcAst *root);
void             ccmmc_draw_symbol_scope            (FILE *fp,
                                                     CcmmcSymbolScope *scope);

#endif
// vim: set sw=4 ts=4 sts=4 et:
