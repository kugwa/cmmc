#ifndef CCMMC_HEADER_DRAW_H
#define CCMMC_HEADER_DRAW_H

#include "ast.h"

#include <stdio.h>

void             ccmmc_draw_ast                     (FILE *fp,
                                                     const char *name,
                                                     CcmmcAst *root);

#endif
// vim: set sw=4 ts=4 sts=4 et:
