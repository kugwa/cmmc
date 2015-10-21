%option noyywrap
%{
#include <string.h>
#include "symbol-table.h"

#define SIZE_OF_ARR(x) (sizeof(x)/sizeof(x[0]))

int linenumber;

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
    CCMMC_TOKEN_ERROR = 100
} CcmmcToken;
%}

letter          [A-Za-z]
digit           [0-9]
ID              {letter}({letter}|{digit}|"_")*
WS              [ \t]+

/* You need to define the following RE's */
CONST_INT       {digit}+
CONST_FLOAT     {digit}+\.{digit}+
/* -?([0-9]+|[0-9]*\.[0-9]+([eE][-+]?[0-9]+)?) */
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

                    for (i = 0; i < SIZE_OF_ARR(reserved); i++)
                        if (strcmp(yytext, reserved[i]) == 0) break;
                    if (i == SIZE_OF_ARR(reserved)) {
                        symtab * ptr;
                        ptr = lookup(yytext);
                        if (ptr == NULL)
                            insertID(yytext);
                        else
                            ptr->counter++;
                    }
                }
{CONST_INT}     {}
{CONST_FLOAT}   {}
{CONST_STRING}  {}

{COMMENT}       puts(yytext);
{OP_ASSIGN}     {}
{OP_OR}         {}
{OP_AND}        {}
{OP_NOT}        {}
{OP_ADD}        {}
{OP_SUB}        {}
{OP_MUL}        {}
{OP_DIV}        {}
{OP_GT}         {}
{OP_LT}         {}
{OP_GE}         {}
{OP_LE}         {}
{OP_NE}         {}
{OP_EQ}         {}

{NEWLINE}       linenumber++;

{DL_LPAREN}     {}
{DL_RPAREN}     {}
{DL_LBRACK}     {}
{DL_RBRACK}     {}
{DL_LBRACE}     {}
{DL_RBRACE}     {}
{DL_COMMA}      {}
{DL_SEMICOL}    {}
{DL_DOT}        {}

{ERROR}         {
                    fprintf(stderr, "%d: error: undefined character `%s'\n",
                        linenumber, yytext);
                }


%%

// vim: set sw=4 ts=4 sts=4 et:
