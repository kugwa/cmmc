%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

AST_NODE *prog;

extern int g_anyErrorOccur;
%}

%union{
	char *lexeme;
	CON_Type  *const1;
	AST_NODE  *node;
};

%token <lexeme>ID
%token <const1>CONST
%token VOID
%token INT
%token FLOAT
%token IF
%token ELSE
%token WHILE
%token FOR
%token TYPEDEF
%token OP_ASSIGN
%token OP_OR
%token OP_AND
%token OP_NOT
%token OP_EQ
%token OP_NE
%token OP_GT
%token OP_LT
%token OP_GE
%token OP_LE
%token OP_ADD
%token OP_SUB
%token OP_MUL
%token OP_DIV
%token DL_LPAREN
%token DL_RPAREN
%token DL_LBRACK
%token DL_RBRACK
%token DL_LBRACE
%token DL_RBRACE
%token DL_COMMA
%token DL_SEMICOL
%token DL_DOT
%token ERROR
%token RETURN

%{
#include "lexer.c"

int yyerror (char *mesg)
{
    fprintf(stderr, "Error found in Line \t%d\tnext token: \t%s\n",
        line_number, yytext);
    exit(1);
}
%}

%type <node> program global_decl_list global_decl function_decl block stmt_list
%type <node> decl_list decl var_decl type init_id_list init_id stmt relop_expr
%type <node> relop_term relop_factor expr term factor var_ref param_list param
%type <node> dim_fn expr_null id_list dim_decl cexpr mcexpr cfactor
%type <node> assign_expr_list test assign_expr rel_op relop_expr_list
%type <node> nonempty_relop_expr_list add_op mul_op dim_list type_decl
%type <node> nonempty_assign_expr_list

%start program

%%

/* ==== Grammar Section ==== */

/* Productions */               /* Semantic actions */
program		: global_decl_list { $$=Allocate(PROGRAM_NODE);  makeChild($$,$1); prog=$$;}
		| { $$=Allocate(PROGRAM_NODE); prog=$$;}
		;

global_decl_list: global_decl_list global_decl
                    {
                        $$ = makeSibling($1, $2);
                    }
                | global_decl
                    {
                        $$ = $1;
                    }
                ;

global_decl	: decl_list function_decl
                {
                    $$ = makeSibling(makeChild(Allocate(VARIABLE_DECL_LIST_NODE), $1), $2);
                }
            | function_decl
                {
                    $$ = $1;
                }
            ;

function_decl	: type ID DL_LPAREN param_list DL_RPAREN DL_LBRACE block DL_RBRACE
                    {
                        $$ = makeDeclNode(FUNCTION_DECL);
                        AST_NODE* parameterList = Allocate(PARAM_LIST_NODE);
                        makeChild(parameterList, $4);
                        makeFamily($$, 4, $1, makeIDNode($2, NORMAL_ID), parameterList, $7);
                    }
                | VOID ID DL_LPAREN param_list DL_RPAREN DL_LBRACE block DL_RBRACE
                    {
                        /*TODO*/
                    }
                | type ID DL_LPAREN  DL_RPAREN DL_LBRACE block DL_RBRACE
                    {
                        $$ = makeDeclNode(FUNCTION_DECL);
                        AST_NODE* emptyParameterList = Allocate(PARAM_LIST_NODE);
                        makeFamily($$, 4, $1, makeIDNode($2, NORMAL_ID), emptyParameterList, $6);
                    }
                | VOID ID DL_LPAREN  DL_RPAREN DL_LBRACE block DL_RBRACE
                    {
                        /*TODO*/
                    }
                ;

param_list	: param_list DL_COMMA  param
                {
                    $$ = makeSibling($1, $3);
                }
            | param
                {
                    /*TODO*/
                }
            ;

param		: type ID
                {
                    $$ = makeDeclNode(FUNCTION_PARAMETER_DECL);
                    makeFamily($$, 2, $1, makeIDNode($2, NORMAL_ID));
                }
            | type ID dim_fn
                {
                    /*TODO*/
                }
            ;
dim_fn		: DL_LBRACK expr_null DL_RBRACK
                {
                    $$ = $2;
                }
            | dim_fn DL_LBRACK expr DL_RBRACK
                {
                    $$ = makeSibling($1, $3);
                }
		;

expr_null	:expr
                {
                    /*TODO*/
                }
            |
                {
                    $$ = Allocate(NUL_NODE);
                }
            ;

block           : decl_list stmt_list
                    {
                        /*TODO*/
                    }
                | stmt_list
                    {
                        $$ = Allocate(BLOCK_NODE);
                        makeChild($$, makeChild(Allocate(STMT_LIST_NODE), $1));
                    }
                | decl_list
                    {
                        $$ = Allocate(BLOCK_NODE);
                        makeChild($$, makeChild(Allocate(VARIABLE_DECL_LIST_NODE), $1));
                    }
                |   {
                        /*TODO*/
                    }
                ;

decl_list	: decl_list decl
                {
                        /*TODO*/
                }
            | decl
                {
                        /*TODO*/
                }
            ;

decl		: type_decl
                {
                    $$ = $1;
                }
            | var_decl
                {
                    $$ = $1;
                }
            ;

type_decl 	: TYPEDEF type id_list DL_SEMICOL
                {
                    /*TODO*/
                }
            | TYPEDEF VOID id_list DL_SEMICOL
                {
                    /*TODO*/
                }
            ;

var_decl	: type init_id_list DL_SEMICOL
                {
                    /*TODO*/
                }
            | ID id_list DL_SEMICOL
                {
                    /*TODO*/
                }
            ;

type		: INT
                {
                    $$ = makeIDNode("int", NORMAL_ID);
                }
            | FLOAT
                {
                    $$ = makeIDNode("float", NORMAL_ID);
                }
            ;

id_list		: ID
                {
                    $$ = makeIDNode($1, NORMAL_ID);
                }
            | id_list DL_COMMA ID
                {
                    /*TODO*/
                }
            | id_list DL_COMMA ID dim_decl
                {
                    /*TODO*/
                }
            | ID dim_decl
                {
                    /*TODO*/
                }
		;
dim_decl	: DL_LBRACK cexpr DL_RBRACK
                {
                    /*TODO*/
                }
            /*TODO: Try if you can define a recursive production rule
            | .......
            */
            ;
cexpr		: cexpr OP_ADD mcexpr
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_ADD);
                    makeFamily($$, 2, $1, $3);
                } /* This is for array declarations */
            | cexpr OP_SUB mcexpr
                {
                    /*TODO*/
                }
            | mcexpr
                {
                    /*TODO*/
                }
            ;
mcexpr		: mcexpr OP_MUL cfactor
                {
                    /*TODO*/
                }
            | mcexpr OP_DIV cfactor
                {
                    /*TODO*/
                }
            | cfactor
                {
                    /*TODO*/
                }
            ;

cfactor:	CONST
                {
                    /*TODO*/
                }
            | DL_LPAREN cexpr DL_RPAREN
                {
                    /*TODO*/
                }
            ;

init_id_list	: init_id
                    {
                        /*TODO*/
                    }
                | init_id_list DL_COMMA init_id
                    {
                        /*TODO*/
                    }
                ;

init_id		: ID
                {
                    $$ = makeIDNode($1, NORMAL_ID);
                }
            | ID dim_decl
                {
                    /*TODO*/
                }
            | ID OP_ASSIGN relop_expr
                {
                    /*TODO*/
                }
            ;

stmt_list	: stmt_list stmt
                {
                    /*TODO*/
                }
            | stmt
                {
                    /*TODO*/
                }
            ;



stmt		: DL_LBRACE block DL_RBRACE
                {
                    /*TODO*/
                }
            /*TODO: | While Statement */
            | FOR DL_LPAREN assign_expr_list DL_SEMICOL relop_expr_list DL_SEMICOL assign_expr_list DL_RPAREN stmt
                {
                    /*TODO*/
                }
            | var_ref OP_ASSIGN relop_expr DL_SEMICOL
                {
                    /*TODO*/
                }
            /*TODO: | If Statement */
            /*TODO: | If then else */
            /*TODO: | function call */
            | DL_SEMICOL
                {
                    /*TODO*/
                }
            | RETURN DL_SEMICOL
                {
                    /*TODO*/
                }
            | RETURN relop_expr DL_SEMICOL
                {
                    /*TODO*/
                }
            ;

assign_expr_list : nonempty_assign_expr_list
                     {
                        /*TODO*/
                     }
                 |
                     {
                         $$ = Allocate(NUL_NODE);
                     }
                 ;

nonempty_assign_expr_list        : nonempty_assign_expr_list DL_COMMA assign_expr
                                    {
                                        /*TODO*/
                                    }
                                 | assign_expr
                                    {
                                        /*TODO*/
                                    }
                                 ;

test		: assign_expr
                {
                    $$ = $1;
                }
            ;

assign_expr     : ID OP_ASSIGN relop_expr
                    {
                        /*TODO*/
                    }
                | relop_expr
                    {
                        /*TODO*/
                    }
		;

relop_expr	: relop_term
                {
                    $$ = $1;
                }
            | relop_expr OP_OR relop_term
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_OR);
                    makeFamily($$, 2, $1, $3);
                }
            ;

relop_term	: relop_factor
                {
                    /*TODO*/
                }
            | relop_term OP_AND relop_factor
                {
                    /*TODO*/
                }
            ;

relop_factor	: expr
                    {
                        /*TODO*/
                    }
                | expr rel_op expr
                    {
                        /*TODO*/
                    }
                ;

rel_op		: OP_EQ
                {
                    /*TODO*/
                }
            | OP_GE
                {
                    /*TODO*/
                }
            | OP_LE
                {
                    /*TODO*/
                }
            | OP_NE
                {
                    /*TODO*/
                }
            | OP_GT
                {
                    /*TODO*/
                }
            | OP_LT
                {
                    /*TODO*/
                }
            ;


relop_expr_list	: nonempty_relop_expr_list
                    {
                        /*TODO*/
                    }
                |
                    {
                        $$ = Allocate(NUL_NODE);
                    }
                ;

nonempty_relop_expr_list	: nonempty_relop_expr_list DL_COMMA relop_expr
                                {
                                    /*TODO*/
                                }
                            | relop_expr
                                {
                                    /*TODO*/
                                }
                            ;

expr		: expr add_op term
                {
                    /*TODO*/
                }
            | term
                {
                    /*TODO*/
                }
            ;

add_op		: OP_ADD
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_ADD);
                }
            | OP_SUB
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_SUB);
                }
            ;

term		: term mul_op factor
                {
                    /*TODO*/
                }
            | factor
                {
                    /*TODO*/
                }
            ;

mul_op		: OP_MUL
                {
                    /*TODO*/
                }
            | OP_DIV
                {
                    /*TODO*/
                }
            ;

factor		: DL_LPAREN relop_expr DL_RPAREN
                {
                    /*TODO*/
                }
            /*TODO: | -(<relop_expr>) e.g. -(4) */
            | OP_NOT DL_LPAREN relop_expr DL_RPAREN
                {
                    /*TODO*/
                }
            | CONST
                {
                    $$ = Allocate(CONST_VALUE_NODE);
                    $$->semantic_value.const1=$1;
                }
            /*TODO: | -<constant> e.g. -4 */
            | OP_NOT CONST
                {
                    /*TODO*/
                }
            | ID DL_LPAREN relop_expr_list DL_RPAREN
                {
                    /*TODO*/
                }
            /*TODO: | -<function call> e.g. -f(4) */
            | OP_NOT ID DL_LPAREN relop_expr_list DL_RPAREN
                {
                    /*TODO*/
                }
            | var_ref
                {
                    /*TODO*/
                }
            /*TODO: | -<var_ref> e.g. -var */
            | OP_NOT var_ref
                {
                    /*TODO*/
                }
            ;

var_ref		: ID
                {
                    /*TODO*/
                }
            | ID dim_list
                {
                    /*TODO*/
                }
            ;


dim_list	: dim_list DL_LBRACK expr DL_RBRACK
                {
                    /*TODO*/
                }
            | DL_LBRACK expr DL_RBRACK
                {
                    /*TODO*/
                }
		;

%%

