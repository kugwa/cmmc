#ifndef CCMMC_HEADER_SEMANTIC_ANALYSIS_H
#define CCMMC_HEADER_SEMANTIC_ANALYSIS_H

#include "ast.h"
#include "symbol-table.h"

#include <stdbool.h>

bool             ccmmc_semantic_check               (CcmmcAst *root,
                                                     CcmmcSymbolTable *table);

#endif
// vim: set sw=4 ts=4 sts=4 et:
