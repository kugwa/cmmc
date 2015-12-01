%option noyywrap reentrant bison-bridge
%option outfile="lex.yy.c"
%option prefix="ccmmc_parser_"
%{
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "ast.h"
#include "common.h"
#include "libparser_a-parser.h"
#include "symbol-table.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int line_number = 1;

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
                    if (i == SIZEOF_ARRAY(reserved)) {
                        CcmmcSymbol *ptr;
                        ptr = ccmmc_symbol_table_lookup(yytext);
                        if (ptr == NULL)
                            ccmmc_symbol_table_insert_id(yytext, line_number);
                        else
                            ptr->counter++;
                    }
                    yylval->lexeme = strdup(yytext);
                    ERR_FATAL_CHECK(yylval->lexeme, strdup);
                    return ID;
                }
{CONST_INT}     {
                    CON_Type *p;
                    p = malloc(sizeof(CON_Type));
                    ERR_FATAL_CHECK(p, malloc);
                    p->const_type = INTEGERC;
                    p->const_u.intval = atoi(yytext);
                    yylval->const1 = p;
                    return CONST;
                }
{CONST_FLOAT}   {
                    CON_Type *p;
                    p = malloc(sizeof(CON_Type));
                    ERR_FATAL_CHECK(p, malloc);
                    p->const_type = FLOATC;
                    p->const_u.fval = atof(yytext);
                    yylval->const1 = p;
                    return CONST;
                }
{CONST_STRING}  {
                    CON_Type *p;
                    p = malloc(sizeof(CON_Type));
                    ERR_FATAL_CHECK(p, malloc);
                    p->const_type = STRINGC;
                    p->const_u.sc = strdup(yytext);
                    ERR_FATAL_CHECK(p->const_u.sc, strdup);
                    yylval->const1 = p;
                    return CONST;
                }
{COMMENT}       {
                    for (size_t i = 0; yytext[i] != '\0'; i++)
                        if (yytext[i] == '\n')
                            line_number++;
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

{NEWLINE}       line_number++;

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
                    fprintf(stderr, "%d: error: undefined character `%s'\n",
                        line_number, yytext);
                    exit(1);
                }

%%

// vim: set sw=4 ts=4 sts=4 et:
