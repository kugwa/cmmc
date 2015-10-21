#include <stdio.h>
#include "lexer.h"
#include "symbol-table.h"

int main(int argc, char **argv)
{
    if (argc > 1)
        yyin = fopen(argv[1], "r");
    else
        yyin = stdin;
    yylex();
    printSymTab();
    return 0;
}
