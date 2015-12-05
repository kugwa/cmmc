%option noyywrap reentrant bison-bridge
%option outfile="lex.yy.c"
%option prefix="ccmmc_parser_"
%{
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "common.h"
#include "state.h"

#include "libparser_a-parser.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define YYSTYPE CCMMC_PARSER_STYPE
%}

letter          [A-Za-z]
digit           [0-9]
ID              {letter}({letter}|{digit}|"_")*
WS              [ \t]+

/* You need to define the following RE's */
CONST_INT       {digit}+
CONST_FLOAT     (([0-9]+\.?|[0-9]*\.[0-9]+)([eE][-+]?[0-9]+)?)
CONST_STRING    \"([^\"\n]|(\\.))*\"
COMMENT         \/\*([^*]|\n|(\*+([^*/]|\n)))*\*+\/

/* operators */
OP_ASSIGN       "="
OP_OR           "||"
OP_AND          "&&"
OP_NOT          "!"
OP_ADD          "+"
OP_SUB          "-"
OP_MUL          "*"
OP_DIV          "/"
OP_GT           ">"
OP_LT           "<"
OP_GE           ">="
OP_LE           "<="
OP_NE           "!="
OP_EQ           "=="

NEWLINE         "\n"

/* separators */
DL_LPAREN       "("
DL_RPAREN       ")"
DL_LBRACK       "["
DL_RBRACK       "]"
DL_LBRACE       "{"
DL_RBRACE       "}"
DL_COMMA        ","
DL_SEMICOL      ";"
DL_DOT          "."

ERROR           .

%%

{WS}            {}
{ID}            {
                    size_t i;
                    char *reserved[] = {"return", "typedef", "if", "else",
                        "int", "float", "for", "void", "while"};
                    enum ccmmc_parser_tokentype reserved_token[] = {RETURN,
                        TYPEDEF, IF, ELSE, INT, FLOAT, FOR, VOID, WHILE};
                    static_assert(
                        SIZEOF_ARRAY(reserved) == SIZEOF_ARRAY(reserved_token),
                        "Reserved words array and reserved tokens array "
                        "must have the same size");

                    for (i = 0; i < SIZEOF_ARRAY(reserved); i++)
                        if (strcmp(yytext, reserved[i]) == 0)
                            return reserved_token[i];
                    yylval->lexeme = strdup(yytext);
                    ERR_FATAL_CHECK(yylval->lexeme, strdup);
                    return ID;
                }
{CONST_INT}     {
                    yylval->value_const.kind = CCMMC_KIND_CONST_INT;
                    yylval->value_const.const_int = atoi(yytext);
                    return CONST;
                }
{CONST_FLOAT}   {
                    yylval->value_const.kind = CCMMC_KIND_CONST_FLOAT;
                    yylval->value_const.const_float = atof(yytext);
                    return CONST;
                }
{CONST_STRING}  {
                    size_t len = strlen(yytext);
                    yylval->value_const.kind = CCMMC_KIND_CONST_STRING;
                    yylval->value_const.const_string = strndup(yytext + 1, len - 2);
                    ERR_FATAL_CHECK(yylval->value_const.const_string, strdup);
                    return CONST;
                }
{COMMENT}       {
                    CcmmcState *state = yyextra;
                    for (size_t i = 0; yytext[i] != '\0'; i++)
                        if (yytext[i] == '\n')
                            state->line_number++;
                }
{OP_ASSIGN}     return OP_ASSIGN;
{OP_OR}         return OP_OR;
{OP_AND}        return OP_AND;
{OP_NOT}        return OP_NOT;
{OP_ADD}        return OP_ADD;
{OP_SUB}        return OP_SUB;
{OP_MUL}        return OP_MUL;
{OP_DIV}        return OP_DIV;
{OP_GT}         return OP_GT;
{OP_LT}         return OP_LT;
{OP_GE}         return OP_GE;
{OP_LE}         return OP_LE;
{OP_NE}         return OP_NE;
{OP_EQ}         return OP_EQ;

{NEWLINE}       {
                    CcmmcState *state = yyextra;
                    state->line_number++;
                }
{DL_LPAREN}     return DL_LPAREN;
{DL_RPAREN}     return DL_RPAREN;
{DL_LBRACK}     return DL_LBRACK;
{DL_RBRACK}     return DL_RBRACK;
{DL_LBRACE}     return DL_LBRACE;
{DL_RBRACE}     return DL_RBRACE;
{DL_COMMA}      return DL_COMMA;
{DL_SEMICOL}    return DL_SEMICOL;
{DL_DOT}        return DL_DOT;

{ERROR}         {
                    CcmmcState *state = yyextra;
                    fprintf(stderr, "%zu: error: undefined character `%s'\n",
                        state->line_number, yytext);
                    exit(1);
                }

%%

// vim: set sw=4 ts=4 sts=4 et:
