#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "symbol-table.h"

static int id_compare(const void *a, const void *b) {
    const CcmmcSymbol *aa = *(const CcmmcSymbol**)a;
    const CcmmcSymbol *bb = *(const CcmmcSymbol**)b;
    return strcmp(aa->lexeme, bb->lexeme);
}

int main(int argc, char **argv) {
    if (argc > 1)
        yyin = fopen(argv[1], "r");
    else
        yyin = stdin;
    yylex();

    int len;
    CcmmcSymbol **id_list = ccmmc_symbol_table_tmp(&len);
    qsort(id_list, len, sizeof(CcmmcSymbol*), id_compare);

    puts("Frequency of identifiers:");
    for (int i = 0; i < len; i++) {
        printf("%-15s %d\n", id_list[i]->lexeme, id_list[i]->counter);
    }

    return 0;
}

// vim: set sw=4 ts=4 sts=4 et:
