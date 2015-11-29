%{
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int yylex(void);
static void yyerror(const char *mesg);

AST_NODE *prog;

extern char *yytext;
extern int line_number;
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

%right DL_RPAREN ELSE

%type <node> program global_decl_list global_decl function_decl block stmt_list
%type <node> decl_list decl var_decl type init_id_list init_id stmt relop_expr
%type <node> relop_term relop_factor expr term factor var_ref param_list param
%type <node> dim_fn expr_null id_list dim_decl cexpr mcexpr cfactor
%type <node> assign_expr_list assign_expr rel_op relop_expr_list
%type <node> nonempty_relop_expr_list add_op mul_op dim_list type_decl
%type <node> nonempty_assign_expr_list

%start program

%%

/* ==== Grammar Section ==== */

/* Productions */               /* Semantic actions */
program     : global_decl_list
                {
                        $$=Allocate(PROGRAM_NODE);
                        makeChild($$,$1);
                        prog=$$;
                }
            |
                {
                        $$=Allocate(PROGRAM_NODE); prog=$$;
                }
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

global_decl : type_decl
                {
                    $$ = $1;
                }
            | var_decl
                {
                    $$ = $1;
                }
            | function_decl
                {
                    $$ = $1;
                }
            ;

function_decl   : type ID DL_LPAREN param_list DL_RPAREN DL_LBRACE block DL_RBRACE
                    {
                        $$ = makeDeclNode(FUNCTION_DECL);
                        AST_NODE* parameterList = Allocate(PARAM_LIST_NODE);
                        makeChild(parameterList, $4);
                        makeFamily($$, 4, $1, makeIDNode($2, NORMAL_ID), parameterList, $7);
                    }
                | VOID ID DL_LPAREN param_list DL_RPAREN DL_LBRACE block DL_RBRACE
                    {
                        $$ = makeDeclNode(FUNCTION_DECL);
                        AST_NODE* parameterList = Allocate(PARAM_LIST_NODE);
                        makeChild(parameterList, $4);
                        makeFamily($$, 4, makeIDNode("void", NORMAL_ID), makeIDNode($2, NORMAL_ID), parameterList, $7);
                    }
                | type ID DL_LPAREN  DL_RPAREN DL_LBRACE block DL_RBRACE
                    {
                        $$ = makeDeclNode(FUNCTION_DECL);
                        AST_NODE* emptyParameterList = Allocate(PARAM_LIST_NODE);
                        makeFamily($$, 4, $1, makeIDNode($2, NORMAL_ID), emptyParameterList, $6);
                    }
                | VOID ID DL_LPAREN  DL_RPAREN DL_LBRACE block DL_RBRACE
                    {
                        $$ = makeDeclNode(FUNCTION_DECL);
                        AST_NODE* emptyParameterList = Allocate(PARAM_LIST_NODE);
                        makeFamily($$, 4, makeIDNode("void", NORMAL_ID), makeIDNode($2, NORMAL_ID), emptyParameterList, $6);
                    }
                ;

param_list  : param_list DL_COMMA  param
                {
                    $$ = makeSibling($1, $3);
                }
            | param
                {
                    $$ = $1;
                }
            ;

param       : type ID
                {
                    $$ = makeDeclNode(FUNCTION_PARAMETER_DECL);
                    makeFamily($$, 2, $1, makeIDNode($2, NORMAL_ID));
                }
            | type ID dim_fn
                {
                    $$ = makeDeclNode(FUNCTION_PARAMETER_DECL);
                    makeFamily($$, 2, $1, makeChild(makeIDNode($2, ARRAY_ID), $3));
                }
            ;
dim_fn      : DL_LBRACK expr_null DL_RBRACK
                {
                    $$ = $2;
                }
            | dim_fn DL_LBRACK expr DL_RBRACK
                {
                    $$ = makeSibling($1, $3);
                }
            ;

expr_null   :expr
                {
                    $$ = $1;
                }
            |
                {
                    $$ = Allocate(NUL_NODE);
                }
            ;

block           : decl_list stmt_list
                    {
                        $$ = Allocate(BLOCK_NODE);
                        makeFamily($$, 2, makeChild(Allocate(VARIABLE_DECL_LIST_NODE), $1), makeChild(Allocate(STMT_LIST_NODE), $2));
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
                        $$ = Allocate(BLOCK_NODE);
                    }
                ;

decl_list   : decl_list decl
                {
                    $$ = makeSibling($1, $2);
                }
            | decl
                {
                    $$ = $1;
                }
            ;

decl        : type_decl
                {
                    $$ = $1;
                }
            | var_decl
                {
                    $$ = $1;
                }
            ;

type_decl   : TYPEDEF type id_list DL_SEMICOL
                {
                    $$ = makeDeclNode(TYPE_DECL);
                    makeFamily($$, 2, $2, $3);
                }
            | TYPEDEF VOID id_list DL_SEMICOL
                {
                    $$ = makeDeclNode(TYPE_DECL);
                    makeFamily($$, 2, makeIDNode("void", NORMAL_ID), $3);
                }
            ;

var_decl    : type init_id_list DL_SEMICOL
                {
                    $$ = makeDeclNode(VARIABLE_DECL);
                    makeFamily($$, 2, $1, $2);
                }
            | ID id_list DL_SEMICOL
                {
                    $$ = makeDeclNode(VARIABLE_DECL);
                    makeFamily($$, 2, makeIDNode($1, NORMAL_ID), $2);
                }
            ;

type        : INT
                {
                    $$ = makeIDNode("int", NORMAL_ID);
                }
            | FLOAT
                {
                    $$ = makeIDNode("float", NORMAL_ID);
                }
            ;

id_list     : ID
                {
                    $$ = makeIDNode($1, NORMAL_ID);
                }
            | id_list DL_COMMA ID
                {
                    $$ = makeSibling($1, makeIDNode($3, NORMAL_ID));
                }
            | id_list DL_COMMA ID dim_decl
                {
                    $$ = makeSibling($1, makeChild(makeIDNode($3, ARRAY_ID), $4));
                }
            | ID dim_decl
                {
                    $$ = makeChild(makeIDNode($1, ARRAY_ID), $2);
                }
            ;
dim_decl    : DL_LBRACK cexpr DL_RBRACK
                {
                    $$ = $2;
                }
            | dim_decl DL_LBRACK cexpr DL_RBRACK
                {
                    $$ = makeSibling($1, $3);
                }
            ;
cexpr       : cexpr OP_ADD mcexpr
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_ADD);
                    makeFamily($$, 2, $1, $3);
                } /* This is for array declarations */
            | cexpr OP_SUB mcexpr
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_SUB);
                    makeFamily($$, 2, $1, $3);
                }
            | mcexpr
                {
                    $$ = $1;
                }
            ;
mcexpr      : mcexpr OP_MUL cfactor
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_MUL);
                    makeFamily($$, 2, $1, $3);
                }
            | mcexpr OP_DIV cfactor
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_DIV);
                    makeFamily($$, 2, $1, $3);
                }
            | cfactor
                {
                    $$ = $1;
                }
            ;

cfactor:    CONST
                {
                    $$ = Allocate(CONST_VALUE_NODE);
                    $$->semantic_value.const1=$1;
                }
            | DL_LPAREN cexpr DL_RPAREN
                {
                    $$ = $2;
                }
            ;

init_id_list    : init_id
                    {
                        $$ = $1;
                    }
                | init_id_list DL_COMMA init_id
                    {
                        $$ = makeSibling($1, $3);
                    }
                ;

init_id     : ID
                {
                    $$ = makeIDNode($1, NORMAL_ID);
                }
            | ID dim_decl
                {
                    $$ = makeChild(makeIDNode($1, ARRAY_ID), $2);
                }
            | ID OP_ASSIGN relop_expr
                {
                    $$ = makeIDNode($1, WITH_INIT_ID);
                    makeChild($$, $3);
                }
            ;

stmt_list   : stmt_list stmt
                {
                    $$ = makeSibling($1, $2);
                }
            | stmt
                {
                    $$ = $1;
                }
            ;



stmt        : DL_LBRACE block DL_RBRACE
                {
                    $$ = $2;
                }
            | WHILE DL_LPAREN relop_expr DL_RPAREN stmt
                {
                    $$ = makeStmtNode(WHILE_STMT);
                    makeFamily($$, 2, $3, $5);
                }
            | FOR DL_LPAREN assign_expr_list DL_SEMICOL relop_expr_list DL_SEMICOL assign_expr_list DL_RPAREN stmt
                {
                    $$ = makeStmtNode(FOR_STMT);
                    makeFamily($$, 4, $3, $5, $7, $9);
                }
            | var_ref OP_ASSIGN relop_expr DL_SEMICOL
                {
                    $$ = makeStmtNode(ASSIGN_STMT);
                    makeFamily($$, 2, $1, $3);
                }
            | IF DL_LPAREN relop_expr DL_RPAREN stmt
                {
                    $$ = makeStmtNode(IF_STMT);
                    makeFamily($$, 3, $3, $5, Allocate(NUL_NODE));
                }
            | IF DL_LPAREN relop_expr DL_RPAREN stmt ELSE stmt
                {
                    $$ = makeStmtNode(IF_STMT);
                    makeFamily($$, 3, $3, $5, $7);
                }
            | ID DL_LPAREN relop_expr_list DL_RPAREN DL_SEMICOL
                {
                    $$ = makeStmtNode(FUNCTION_CALL_STMT);
                    makeFamily($$, 2, makeIDNode($1, NORMAL_ID), $3);
                }
            | DL_SEMICOL
                {
                    $$ = Allocate(NUL_NODE);
                }
            | RETURN DL_SEMICOL
                {
                    $$ = makeStmtNode(RETURN_STMT);
                }
            | RETURN relop_expr DL_SEMICOL
                {
                    $$ = makeStmtNode(RETURN_STMT);
                    makeChild($$, $2);
                }
            ;

assign_expr_list : nonempty_assign_expr_list
                     {
                         $$ = Allocate(NONEMPTY_ASSIGN_EXPR_LIST_NODE);
                         makeChild($$, $1);
                     }
                 |
                     {
                         $$ = Allocate(NUL_NODE);
                     }
                 ;

nonempty_assign_expr_list        : nonempty_assign_expr_list DL_COMMA assign_expr
                                    {
                                        $$ = makeSibling($1, $3);
                                    }
                                 | assign_expr
                                    {
                                        $$ = $1;
                                    }
                                 ;

assign_expr     : ID OP_ASSIGN relop_expr
                    {
                        $$ = makeStmtNode(ASSIGN_STMT);
                        makeFamily($$, 2, makeIDNode($1, NORMAL_ID), $3);
                    }
                | relop_expr
                    {
                        $$ = $1;
                    }
                ;

relop_expr  : relop_term
                {
                    $$ = $1;
                }
            | relop_expr OP_OR relop_term
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_OR);
                    makeFamily($$, 2, $1, $3);
                }
            ;

relop_term  : relop_factor
                {
                    $$ = $1;
                }
            | relop_term OP_AND relop_factor
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_AND);
                    makeFamily($$, 2, $1, $3);
                }
            ;

relop_factor    : expr
                    {
                        $$ = $1;
                    }
                | expr rel_op expr
                    {
                        $$ = $2;
                        makeFamily($$, 2, $1, $3);
                    }
                ;

rel_op      : OP_EQ
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_EQ);
                }
            | OP_GE
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_GE);
                }
            | OP_LE
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_LE);
                }
            | OP_NE
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_NE);
                }
            | OP_GT
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_GT);
                }
            | OP_LT
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_LT);
                }
            ;


relop_expr_list : nonempty_relop_expr_list
                    {
                        $$ = Allocate(NONEMPTY_RELOP_EXPR_LIST_NODE);
                        makeChild($$, $1);
                    }
                |
                    {
                        $$ = Allocate(NUL_NODE);
                    }
                ;

nonempty_relop_expr_list    : nonempty_relop_expr_list DL_COMMA relop_expr
                                {
                                    $$ = makeSibling($1, $3);
                                }
                            | relop_expr
                                {
                                    $$ = $1;
                                }
                            ;

expr        : expr add_op term
                {
                    $$ = $2;
                    makeFamily($$, 2, $1, $3);
                }
            | term
                {
                    $$ = $1;
                }
            ;

add_op      : OP_ADD
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_ADD);
                }
            | OP_SUB
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_SUB);
                }
            ;

term        : term mul_op factor
                {
                    $$ = $2;
                    makeFamily($$, 2, $1, $3);
                }
            | factor
                {
                    $$ = $1;
                }
            ;

mul_op      : OP_MUL
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_MUL);
                }
            | OP_DIV
                {
                    $$ = makeExprNode(BINARY_OPERATION, BINARY_OP_DIV);
                }
            ;

factor      : DL_LPAREN relop_expr DL_RPAREN
                {
                    $$ = $2;
                }
            | OP_ADD DL_LPAREN relop_expr DL_RPAREN
                {
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_POSITIVE);
                    makeChild($$, $3);
                }
            | OP_SUB DL_LPAREN relop_expr DL_RPAREN
                {
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_NEGATIVE);
                    makeChild($$, $3);
                }
            | OP_NOT DL_LPAREN relop_expr DL_RPAREN
                {
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_LOGICAL_NEGATION);
                    makeChild($$, $3);
                }
            | CONST
                {
                    $$ = Allocate(CONST_VALUE_NODE);
                    $$->semantic_value.const1=$1;
                }
            | OP_ADD CONST
                {
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_POSITIVE);
                    AST_NODE *const_node = Allocate(CONST_VALUE_NODE);
                    const_node->semantic_value.const1 = $2;
                    makeChild($$, const_node);
                }
            | OP_SUB CONST
                {
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_NEGATIVE);
                    AST_NODE *const_node = Allocate(CONST_VALUE_NODE);
                    const_node->semantic_value.const1 = $2;
                    makeChild($$, const_node);
                }
            | OP_NOT CONST
                {
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_LOGICAL_NEGATION);
                    AST_NODE *const_node = Allocate(CONST_VALUE_NODE);
                    const_node->semantic_value.const1 = $2;
                    makeChild($$, const_node);
                }
            | ID DL_LPAREN relop_expr_list DL_RPAREN
                {
                    $$ = makeStmtNode(FUNCTION_CALL_STMT);
                    makeFamily($$, 2, makeIDNode($1, NORMAL_ID), $3);
                }
            | OP_ADD ID DL_LPAREN relop_expr_list DL_RPAREN
                {
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_POSITIVE);
                    AST_NODE *func_node = makeStmtNode(FUNCTION_CALL_STMT);
                    makeFamily(func_node, 2, makeIDNode($2, NORMAL_ID), $4);
                    makeChild($$, func_node);
                }
            | OP_SUB ID DL_LPAREN relop_expr_list DL_RPAREN
                {
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_NEGATIVE);
                    AST_NODE *func_node = makeStmtNode(FUNCTION_CALL_STMT);
                    makeFamily(func_node, 2, makeIDNode($2, NORMAL_ID), $4);
                    makeChild($$, func_node);
                }
            | OP_NOT ID DL_LPAREN relop_expr_list DL_RPAREN
                {
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_LOGICAL_NEGATION);
                    AST_NODE *func_node = makeStmtNode(FUNCTION_CALL_STMT);
                    makeFamily(func_node, 2, makeIDNode($2, NORMAL_ID), $4);
                    makeChild($$, func_node);
                }
            | var_ref
                {
                    $$ = $1;
                }
            | OP_ADD var_ref
                {
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_POSITIVE);
                    makeChild($$, $2);
                }
            | OP_SUB var_ref
                {
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_NEGATIVE);
                    makeChild($$, $2);
                }
            | OP_NOT var_ref
                {
                    $$ = makeExprNode(UNARY_OPERATION, UNARY_OP_LOGICAL_NEGATION);
                    makeChild($$, $2);
                }
            ;

var_ref     : ID
                {
                    $$ = makeIDNode($1, NORMAL_ID);
                }
            | ID dim_list
                {
                    $$ = makeIDNode($1, ARRAY_ID);
                    makeChild($$, $2);
                }
            ;


dim_list    : dim_list DL_LBRACK expr DL_RBRACK
                {
                    $$ = makeSibling($1, $3);
                }
            | DL_LBRACK expr DL_RBRACK
                {
                    $$ = $2;
                }
            ;

%%

static void yyerror(const char *mesg)
{
    fprintf(stderr, "Error found in Line \t%d\tnext token: \t%s\n",
        line_number, yytext);
    exit(1);
}

// vim: set sw=4 ts=4 sts=4 et:
