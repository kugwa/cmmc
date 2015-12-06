#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

typedef void* yyscan_t;

#include "ast.h"
#include "common.h"
#include "draw.h"
#include "semantic-analysis.h"
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
    prog_name = strrchr (argv[0], '/');
    prog_name = prog_name == NULL ? prog_name : prog_name + 1;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s SOURCE\n", prog_name);
        exit(1);
    }

    const char *source_name = argv[1];
    FILE *source_handle =
        strcmp(source_name, "-") == 0 ? stdin : fopen(source_name, "r");
    if (source_handle == NULL) {
        fprintf(stderr, "%s: %s: %s\n", prog_name, source_name, ERR_MSG);
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
            fprintf(stderr, "%s: failed because of invalid input\n", prog_name);
            exit(1);
        case 2:
            fprintf(stderr, "%s: failed because of memory exhaustion\n", prog_name);
            exit(1);
        default:
            ; // silence warnings
    }
    ccmmc_parser_lex_destroy(scanner);

    // Dump the AST
    const char *dump_ast = getenv("CCMMC_DUMP_AST");
    if (dump_ast != NULL && *dump_ast != '\0')
        ccmmc_draw_ast(stdout, source_name, state->ast);

    CcmmcSymbolTable table_struct;
    state->table = &table_struct;
    ccmmc_symbol_table_init(state->table);

    bool check_succeeded = ccmmc_semantic_check(state->ast, state->table);
    const char *dump_symbol = getenv("CCMMC_DUMP_SYMBOL");
    if (dump_symbol != NULL && *dump_symbol != '\0') {
        CcmmcSymbolScope *scope = state->table->all;
        for (unsigned int i = 0; scope != NULL; scope = scope->all_next, i++) {
            if (i == 0)
                puts("  * Global Scope *");
            else
                printf("  * Scope %u *\n", i);
            ccmmc_draw_symbol_scope(stdout, scope);
        }
    }
    if (check_succeeded)
        puts("Parsing completed. No errors found.");
    else
        exit(1);

    ccmmc_state_fini(state);
    fclose(source_handle);
    return 0;
}

// vim: set sw=4 ts=4 sts=4 et:
