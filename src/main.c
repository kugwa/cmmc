#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

typedef void* yyscan_t;

#include "ast.h"
#include "common.h"
#include "state.h"

#include "libparser_a-parser.h"

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern int ccmmc_parser_lex_init_extra(void *extra, yyscan_t *scanner);
extern int ccmmc_parser_lex_destroy(yyscan_t *scanner);
extern int ccmmc_parser_set_in(FILE *source_handle, yyscan_t scanner);

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

    const char *source_name = argv[1];
    FILE *source_handle = fopen(source_name, "r");
    if (source_handle == NULL) {
        fprintf(stderr, "%s: %s: %s\n", name, source_name, ERR_MSG);
        exit(1);
    }

    CcmmcState state_struct;
    CcmmcState *state = &state_struct;
    ccmmc_state_init(state);

    yyscan_t scanner;
    ccmmc_parser_lex_init_extra(state, &scanner);
    ccmmc_parser_set_in(source_handle, scanner);
    switch (ccmmc_parser_parse(scanner, state)) {
        case 1:
            fprintf(stderr, "%s: failed because of invalid input\n", name);
            exit(1);
        case 2:
            fprintf(stderr, "%s: failed because of memory exhaustion\n", name);
            exit(1);
        default:
            ; // silence warnings
    }
    ccmmc_parser_lex_destroy(scanner);

    printGV(state->ast, NULL);

    ccmmc_state_fini(state);
    fclose(source_handle);
    return 0;
}

// vim: set sw=4 ts=4 sts=4 et:
