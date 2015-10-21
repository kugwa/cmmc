#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "symbol-table.h"

static int id_compare(const void *a, const void *b) {
    const symtab *aa = *(const symtab**)a;
    const symtab *bb = *(const symtab**)b;
    return strcmp(aa->lexeme, bb->lexeme);
}

int main(int argc, char **argv) {
    if (argc > 1)
        yyin = fopen(argv[1], "r");
    else
        yyin = stdin;
    yylex();

    int len;
    symtab **id_list = fillTab(&len);
    qsort(id_list, len, sizeof(symtab*), id_compare);

    puts("Frequency of identifiers:");
    for (int i = 0; i < len; i++) {
        printf("%-16s%d\n", id_list[i]->lexeme, id_list[i]->counter);
    }

    return 0;
}

// vim: set sw=4 ts=4 sts=4 et:
