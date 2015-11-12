#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "src/libparser_a-parser.h"

extern FILE *yyin;
extern AST_NODE *prog;

void printGV(AST_NODE *root, char* fileName);

int main (int argc, char **argv)
{
    if (argc != 2) {
        fputs("usage: parser [source file]\n", stderr);
        exit(1);
    }
    yyin = fopen(argv[1],"r");
    if (yyin == NULL) {
        fputs("Error opening source file.\n", stderr);
        exit(1);
    }
    yyparse();
    printGV(prog, NULL);
    return 0;
}

// vim: set sw=4 ts=4 sts=4 et:
