#ifndef CCMMC_HEADER_CODE_GENERATION_H
#define CCMMC_HEADER_CODE_GENERATION_H

#include "ast.h"
#include "symbol-table.h"

#include <stdio.h>

void             ccmmc_code_generation              (CcmmcAst *root,
                                                     CcmmcSymbolTable *table,
                                                     FILE *asm_output);

#endif
// vim: set sw=4 ts=4 sts=4 et:
