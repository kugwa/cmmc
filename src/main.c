#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "ast.h"
#include "common.h"
#include "libparser_a-parser.h"

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern FILE *yyin;
extern AST_NODE *prog;

const char *ccmmc_main_name;

int main (int argc, char **argv)
{
    ERR_DECL;
    setlocale (LC_ALL, "");
    name = strrchr (argv[0], '/');
    name = name == NULL ? name : name + 1;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s SOURCE\n", name);
        exit(1);
    }

    const char *filename = argv[1];
    yyin = fopen(filename, "r");
    if (yyin == NULL) {
        fprintf(stderr, "%s: %s: %s\n", name, filename, ERR_MSG);
        exit(1);
    }

    yyparse();
    printGV(prog, NULL);
    return 0;
}

// vim: set sw=4 ts=4 sts=4 et:
