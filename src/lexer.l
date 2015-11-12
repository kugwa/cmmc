%option noyywrap
%{
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "symbol-table.h"

#define SIZE_OF_ARR(x) (sizeof(x)/sizeof(x[0]))

int line_number = 1;
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
                    enum yytokentype reserved_token[] = {RETURN, TYPEDEF,
                        IF, ELSE, INT, FLOAT, FOR, VOID, WHILE};
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
                    return ID;
                }
{CONST_INT}     {
                    CON_Type *p;
                    p = (CON_Type *)malloc(sizeof(CON_Type));
                    p->const_type = INTEGERC;
                    p->const_u.intval = atoi(yytext);
                    yylval.const1 = p;
                    return CONST;
                }
{CONST_FLOAT}   {
                    CON_Type *p;
                    p = (CON_Type *)malloc(sizeof(CON_Type));
                    p->const_type = FLOATC;
                    p->const_u.fval = atof(yytext);
                    yylval.const1 = p;
                    return CONST;
                }
{CONST_STRING}  {
                    CON_Type *p;
                    p = (CON_Type *)malloc(sizeof(CON_Type));
                    p->const_type = STRINGC;
                    p->const_u.sc = strdup(yytext);
                    yylval.const1 = p;
                    return CONST;
                }
{COMMENT}       {
                    int i;
                    for (i = 0; yytext[i]; i++)
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
