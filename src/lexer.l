%option noyywrap
%{
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "symbol-table.h"

#define SIZE_OF_ARR(x) (sizeof(x)/sizeof(x[0]))

static int line_number = 1;

/* You need to define for all tokens in C--, here are some examples */
typedef enum CcmmcToken_enum {
    CCMMC_TOKEN_ID = 1,
    CCMMC_TOKEN_CONST_INT,
    CCMMC_TOKEN_CONST_FLOAT,
    CCMMC_TOKEN_CONST_STRING,
    CCMMC_TOKEN_COMMENT,
    CCMMC_TOKEN_OP_ASSIGN,
    CCMMC_TOKEN_OP_OR,
    CCMMC_TOKEN_OP_AND,
    CCMMC_TOKEN_OP_NOT,
    CCMMC_TOKEN_OP_ADD,
    CCMMC_TOKEN_OP_SUB,
    CCMMC_TOKEN_OP_MUL,
    CCMMC_TOKEN_OP_DIV,
    CCMMC_TOKEN_OP_GT,
    CCMMC_TOKEN_OP_LT,
    CCMMC_TOKEN_OP_GE,
    CCMMC_TOKEN_OP_LE,
    CCMMC_TOKEN_OP_NE,
    CCMMC_TOKEN_OP_EQ,
    CCMMC_TOKEN_DL_LPAREN,
    CCMMC_TOKEN_DL_RPAREN,
    CCMMC_TOKEN_DL_LBRACK,
    CCMMC_TOKEN_DL_RBRACK,
    CCMMC_TOKEN_DL_LBRACE,
    CCMMC_TOKEN_DL_RBRACE,
    CCMMC_TOKEN_DL_COMMA,
    CCMMC_TOKEN_DL_SEMICOL,
    CCMMC_TOKEN_DL_DOT,
    CCMMC_TOKEN_NEWLINE,
    CCMMC_TOKEN_RETURN,
    CCMMC_TOKEN_TYPEDEF,
    CCMMC_TOKEN_IF,
    CCMMC_TOKEN_ELSE,
    CCMMC_TOKEN_INT,
    CCMMC_TOKEN_FLOAT,
    CCMMC_TOKEN_FOR,
    CCMMC_TOKEN_VOID,
    CCMMC_TOKEN_WHILE,
    CCMMC_TOKEN_ERROR = 100
} CcmmcToken;
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
                    int i;
                    char *reserved[] = {"return", "typedef", "if", "else",
                        "int", "float", "for", "void", "while"};
                    CcmmcToken reserved_token[] = {CCMMC_TOKEN_RETURN,
                        CCMMC_TOKEN_TYPEDEF,
                        CCMMC_TOKEN_IF,
                        CCMMC_TOKEN_ELSE,
                        CCMMC_TOKEN_INT,
                        CCMMC_TOKEN_FLOAT,
                        CCMMC_TOKEN_FOR,
                        CCMMC_TOKEN_VOID,
                        CCMMC_TOKEN_WHILE};
                    static_assert(
                        SIZE_OF_ARR(reserved) == SIZE_OF_ARR(reserved_token),
                        "Reserved words array and reserved tokens array "
                        "must have the same size");

                    for (i = 0; i < SIZE_OF_ARR(reserved); i++)
                        if (strcmp(yytext, reserved[i]) == 0)
                            return reserved_token[i];
                    if (i == SIZE_OF_ARR(reserved)) {
                        CcmmcSymbol * ptr;
                        ptr = ccmmc_symbol_table_lookup(yytext);
                        if (ptr == NULL)
                            ccmmc_symbol_table_insert_id(yytext, line_number);
                        else
                            ptr->counter++;
                    }
                    return CCMMC_TOKEN_ID;
                }
{CONST_INT}     return CCMMC_TOKEN_CONST_INT;
{CONST_FLOAT}   return CCMMC_TOKEN_CONST_FLOAT;
{CONST_STRING}  return CCMMC_TOKEN_CONST_STRING;

{COMMENT}       {
                    int i;
                    for (i = 0; yytext[i]; i++)
                        if (yytext[i] == '\n')
                            line_number++;
                }
{OP_ASSIGN}     return CCMMC_TOKEN_OP_ASSIGN;
{OP_OR}         return CCMMC_TOKEN_OP_OR;
{OP_AND}        return CCMMC_TOKEN_OP_AND;
{OP_NOT}        return CCMMC_TOKEN_OP_NOT;
{OP_ADD}        return CCMMC_TOKEN_OP_ADD;
{OP_SUB}        return CCMMC_TOKEN_OP_SUB;
{OP_MUL}        return CCMMC_TOKEN_OP_MUL;
{OP_DIV}        return CCMMC_TOKEN_OP_DIV;
{OP_GT}         return CCMMC_TOKEN_OP_GT;
{OP_LT}         return CCMMC_TOKEN_OP_LT;
{OP_GE}         return CCMMC_TOKEN_OP_GE;
{OP_LE}         return CCMMC_TOKEN_OP_LE;
{OP_NE}         return CCMMC_TOKEN_OP_NE;
{OP_EQ}         return CCMMC_TOKEN_OP_EQ;

{NEWLINE}       line_number++;

{DL_LPAREN}     return CCMMC_TOKEN_DL_LPAREN;
{DL_RPAREN}     return CCMMC_TOKEN_DL_RPAREN;
{DL_LBRACK}     return CCMMC_TOKEN_DL_LBRACK;
{DL_RBRACK}     return CCMMC_TOKEN_DL_RBRACK;
{DL_LBRACE}     return CCMMC_TOKEN_DL_RBRACE;
{DL_RBRACE}     return CCMMC_TOKEN_DL_LBRACE;
{DL_COMMA}      return CCMMC_TOKEN_DL_COMMA;
{DL_SEMICOL}    return CCMMC_TOKEN_DL_SEMICOL;
{DL_DOT}        return CCMMC_TOKEN_DL_DOT;

{ERROR}         {
                    fprintf(stderr, "%d: error: undefined character `%s'\n",
                        line_number, yytext);
                    exit(1);
                }


%%

// vim: set sw=4 ts=4 sts=4 et:
