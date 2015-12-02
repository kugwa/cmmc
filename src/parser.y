%define api.prefix {ccmmc_parser_}
%define api.pure full
%lex-param {yyscan_t scanner}
%parse-param {yyscan_t scanner} {CcmmcState *state}
%{
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

typedef void* yyscan_t;

#include "ast.h"
#include "state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
%}

%union{
    char *lexeme;
    CcmmcAst *node;
    CcmmcValueConst value_const;
};

%{
extern int ccmmc_parser_lex(CCMMC_PARSER_STYPE *yylval, yyscan_t scanner);
extern char *ccmmc_parser_get_text(yyscan_t scanner);
static void ccmmc_parser_error(yyscan_t scanner, CcmmcState *state, const char *mesg);
%}

%token <lexeme>ID
%token <value_const>CONST
%token VOID
%token INT
%token FLOAT
%token TYPEDEF
%token IF
%token ELSE
%token WHILE
%token FOR
%token RETURN
%token OP_ASSIGN
%token OP_OR
%token OP_AND
%token OP_NOT
%token OP_ADD
%token OP_SUB
%token OP_MUL
%token OP_DIV
%token OP_GT
%token OP_LT
%token OP_GE
%token OP_LE
%token OP_NE
%token OP_EQ
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

%right DL_RPAREN ELSE

%type <node> program
%type <node> global_decl_list global_decl
%type <node> function_decl param_list param dim_fn expr_null block
%type <node> decl_list decl type_decl var_decl type
%type <node> id_list dim_decl cexpr mcexpr cfactor
%type <node> init_id_list init_id stmt_list stmt
%type <node> assign_expr_list nonempty_assign_expr_list assign_expr
%type <node> relop_expr relop_term relop_factor rel_op
%type <node> relop_expr_list nonempty_relop_expr_list
%type <node> expr add_op term mul_op factor var_ref dim_list

%start program

%%

/* ==== Grammar Section ==== */

program         : global_decl_list
                    {
                        $$ = ccmmc_ast_new(CCMMC_AST_NODE_PROGRAM);
                        ccmmc_ast_append_child($$, $1);
                        state->ast = $$;
                    }
                |
                    {
                        $$ = ccmmc_ast_new(CCMMC_AST_NODE_PROGRAM);
                        state->ast = $$;
                    }
                ;

global_decl_list: global_decl_list global_decl
                    {
                        $$ = ccmmc_ast_append_sibling($1, $2);
                    }
                | global_decl
                    {
                        $$ = $1;
                    }
                ;

global_decl     : type_decl
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
                        $$ = ccmmc_ast_new_decl(CCMMC_KIND_DECL_FUNCTION);
                        CcmmcAst *param_list = ccmmc_ast_new(
                            CCMMC_AST_NODE_PARAM_LIST);
                        ccmmc_ast_append_child(param_list, $4);
                        ccmmc_ast_append_children($$, 4, $1,
                            ccmmc_ast_new_id($2, CCMMC_KIND_ID_NORMAL),
                            param_list, $7);
                    }
                | VOID ID DL_LPAREN param_list DL_RPAREN DL_LBRACE block DL_RBRACE
                    {
                        $$ = ccmmc_ast_new_decl(CCMMC_KIND_DECL_FUNCTION);
                        CcmmcAst *param_list = ccmmc_ast_new(
                            CCMMC_AST_NODE_PARAM_LIST);
                        ccmmc_ast_append_child(param_list, $4);
                        ccmmc_ast_append_children($$, 4,
                            ccmmc_ast_new_id("void", CCMMC_KIND_ID_NORMAL),
                            ccmmc_ast_new_id($2, CCMMC_KIND_ID_NORMAL),
                            param_list, $7);
                    }
                | type ID DL_LPAREN  DL_RPAREN DL_LBRACE block DL_RBRACE
                    {
                        $$ = ccmmc_ast_new_decl(CCMMC_KIND_DECL_FUNCTION);
                        CcmmcAst *empty_param_list = ccmmc_ast_new(
                            CCMMC_AST_NODE_PARAM_LIST);
                        ccmmc_ast_append_children($$, 4, $1,
                            ccmmc_ast_new_id($2, CCMMC_KIND_ID_NORMAL),
                            empty_param_list, $6);
                    }
                | VOID ID DL_LPAREN  DL_RPAREN DL_LBRACE block DL_RBRACE
                    {
                        $$ = ccmmc_ast_new_decl(CCMMC_KIND_DECL_FUNCTION);
                        CcmmcAst *empty_param_list = ccmmc_ast_new(
                            CCMMC_AST_NODE_PARAM_LIST);
                        ccmmc_ast_append_children($$, 4,
                            ccmmc_ast_new_id("void", CCMMC_KIND_ID_NORMAL),
                            ccmmc_ast_new_id($2, CCMMC_KIND_ID_NORMAL),
                            empty_param_list, $6);
                    }
                ;

param_list  : param_list DL_COMMA  param
                {
                    $$ = ccmmc_ast_append_sibling($1, $3);
                }
            | param
                {
                    $$ = $1;
                }
            ;

param       : type ID
                {
                    $$ = ccmmc_ast_new_decl(CCMMC_KIND_DECL_FUNCTION_PARAMETER);
                    ccmmc_ast_append_children($$, 2, $1,
                        ccmmc_ast_new_id($2, CCMMC_KIND_ID_NORMAL));
                }
            | type ID dim_fn
                {
                    $$ = ccmmc_ast_new_decl(CCMMC_KIND_DECL_FUNCTION_PARAMETER);
                    ccmmc_ast_append_children($$, 2, $1,
                        ccmmc_ast_append_child(
                            ccmmc_ast_new_id($2, CCMMC_KIND_ID_ARRAY), $3));
                }
            ;

dim_fn      : DL_LBRACK expr_null DL_RBRACK
                {
                    $$ = $2;
                }
            | dim_fn DL_LBRACK expr DL_RBRACK
                {
                    $$ = ccmmc_ast_append_sibling($1, $3);
                }
            ;

expr_null   :expr
                {
                    $$ = $1;
                }
            |
                {
                    $$ = ccmmc_ast_new(CCMMC_AST_NODE_NUL);
                }
            ;

block       : decl_list stmt_list
                {
                    $$ = ccmmc_ast_new(CCMMC_AST_NODE_BLOCK);
                    ccmmc_ast_append_children($$, 2,
                        ccmmc_ast_append_child(
                            ccmmc_ast_new(CCMMC_AST_NODE_VARIABLE_DECL_LIST), $1),
                        ccmmc_ast_append_child(
                            ccmmc_ast_new(CCMMC_AST_NODE_STMT_LIST), $2));
                }
            | stmt_list
                {
                    $$ = ccmmc_ast_new(CCMMC_AST_NODE_BLOCK);
                    ccmmc_ast_append_child($$,
                        ccmmc_ast_append_child(
                            ccmmc_ast_new(CCMMC_AST_NODE_STMT_LIST), $1));
                }
            | decl_list
                {
                    $$ = ccmmc_ast_new(CCMMC_AST_NODE_BLOCK);
                    ccmmc_ast_append_child($$,
                        ccmmc_ast_append_child(
                            ccmmc_ast_new(CCMMC_AST_NODE_VARIABLE_DECL_LIST), $1));
                }
            |   {
                    $$ = ccmmc_ast_new(CCMMC_AST_NODE_BLOCK);
                }
            ;

decl_list   : decl_list decl
                {
                    $$ = ccmmc_ast_append_sibling($1, $2);
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
                    $$ = ccmmc_ast_new_decl(CCMMC_KIND_DECL_TYPE);
                    ccmmc_ast_append_children($$, 2, $2, $3);
                }
            | TYPEDEF VOID id_list DL_SEMICOL
                {
                    $$ = ccmmc_ast_new_decl(CCMMC_KIND_DECL_TYPE);
                    ccmmc_ast_append_children($$, 2, ccmmc_ast_new_id("void", CCMMC_KIND_ID_NORMAL), $3);
                }
            ;

var_decl    : type init_id_list DL_SEMICOL
                {
                    $$ = ccmmc_ast_new_decl(CCMMC_KIND_DECL_VARIABLE);
                    ccmmc_ast_append_children($$, 2, $1, $2);
                }
            | ID id_list DL_SEMICOL
                {
                    $$ = ccmmc_ast_new_decl(CCMMC_KIND_DECL_VARIABLE);
                    ccmmc_ast_append_children($$, 2, ccmmc_ast_new_id($1, CCMMC_KIND_ID_NORMAL), $2);
                }
            ;

type        : INT
                {
                    $$ = ccmmc_ast_new_id("int", CCMMC_KIND_ID_NORMAL);
                }
            | FLOAT
                {
                    $$ = ccmmc_ast_new_id("float", CCMMC_KIND_ID_NORMAL);
                }
            ;

id_list     : ID
                {
                    $$ = ccmmc_ast_new_id($1, CCMMC_KIND_ID_NORMAL);
                }
            | id_list DL_COMMA ID
                {
                    $$ = ccmmc_ast_append_sibling($1, ccmmc_ast_new_id($3, CCMMC_KIND_ID_NORMAL));
                }
            | id_list DL_COMMA ID dim_decl
                {
                    $$ = ccmmc_ast_append_sibling($1, ccmmc_ast_append_child(ccmmc_ast_new_id($3, CCMMC_KIND_ID_ARRAY), $4));
                }
            | ID dim_decl
                {
                    $$ = ccmmc_ast_append_child(ccmmc_ast_new_id($1, CCMMC_KIND_ID_ARRAY), $2);
                }
            ;

dim_decl    : DL_LBRACK cexpr DL_RBRACK
                {
                    $$ = $2;
                }
            | dim_decl DL_LBRACK cexpr DL_RBRACK
                {
                    $$ = ccmmc_ast_append_sibling($1, $3);
                }
            ;

cexpr       : cexpr OP_ADD mcexpr
                {
                    $$ = ccmmc_ast_new_expr(CCMMC_KIND_EXPR_BINARY_OP, CCMMC_KIND_OP_BINARY_ADD);
                    ccmmc_ast_append_children($$, 2, $1, $3);
                } /* This is for array declarations */
            | cexpr OP_SUB mcexpr
                {
                    $$ = ccmmc_ast_new_expr(CCMMC_KIND_EXPR_BINARY_OP, CCMMC_KIND_OP_BINARY_SUB);
                    ccmmc_ast_append_children($$, 2, $1, $3);
                }
            | mcexpr
                {
                    $$ = $1;
                }
            ;

mcexpr      : mcexpr OP_MUL cfactor
                {
                    $$ = ccmmc_ast_new_expr(CCMMC_KIND_EXPR_BINARY_OP, CCMMC_KIND_OP_BINARY_MUL);
                    ccmmc_ast_append_children($$, 2, $1, $3);
                }
            | mcexpr OP_DIV cfactor
                {
                    $$ = ccmmc_ast_new_expr(CCMMC_KIND_EXPR_BINARY_OP, CCMMC_KIND_OP_BINARY_DIV);
                    ccmmc_ast_append_children($$, 2, $1, $3);
                }
            | cfactor
                {
                    $$ = $1;
                }
            ;

cfactor:    CONST
                {
                    $$ = ccmmc_ast_new(CCMMC_AST_NODE_CONST_VALUE);
                    $$->value_const = $1;
                }
            | DL_LPAREN cexpr DL_RPAREN
                {
                    $$ = $2;
                }
            ;

init_id_list: init_id
                {
                    $$ = $1;
                }
            | init_id_list DL_COMMA init_id
                {
                    $$ = ccmmc_ast_append_sibling($1, $3);
                }
            ;

init_id     : ID
                {
                    $$ = ccmmc_ast_new_id($1, CCMMC_KIND_ID_NORMAL);
                }
            | ID dim_decl
                {
                    $$ = ccmmc_ast_append_child(
                        ccmmc_ast_new_id($1, CCMMC_KIND_ID_ARRAY), $2);
                }
            | ID OP_ASSIGN relop_expr
                {
                    $$ = ccmmc_ast_new_id($1, CCMMC_KIND_ID_WITH_INIT);
                    ccmmc_ast_append_child($$, $3);
                }
            ;

stmt_list   : stmt_list stmt
                {
                    $$ = ccmmc_ast_append_sibling($1, $2);
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
                    $$ = ccmmc_ast_new_stmt(CCMMC_KIND_STMT_WHILE);
                    ccmmc_ast_append_children($$, 2, $3, $5);
                }
            | FOR DL_LPAREN assign_expr_list DL_SEMICOL relop_expr_list DL_SEMICOL assign_expr_list DL_RPAREN stmt
                {
                    $$ = ccmmc_ast_new_stmt(CCMMC_KIND_STMT_FOR);
                    ccmmc_ast_append_children($$, 4, $3, $5, $7, $9);
                }
            | var_ref OP_ASSIGN relop_expr DL_SEMICOL
                {
                    $$ = ccmmc_ast_new_stmt(CCMMC_KIND_STMT_ASSIGN);
                    ccmmc_ast_append_children($$, 2, $1, $3);
                }
            | IF DL_LPAREN relop_expr DL_RPAREN stmt
                {
                    $$ = ccmmc_ast_new_stmt(CCMMC_KIND_STMT_IF);
                    ccmmc_ast_append_children($$, 3, $3, $5,
                        ccmmc_ast_new(CCMMC_AST_NODE_NUL));
                }
            | IF DL_LPAREN relop_expr DL_RPAREN stmt ELSE stmt
                {
                    $$ = ccmmc_ast_new_stmt(CCMMC_KIND_STMT_IF);
                    ccmmc_ast_append_children($$, 3, $3, $5, $7);
                }
            | ID DL_LPAREN relop_expr_list DL_RPAREN DL_SEMICOL
                {
                    $$ = ccmmc_ast_new_stmt(CCMMC_KIND_STMT_FUNCTION_CALL);
                    ccmmc_ast_append_children($$, 2,
                        ccmmc_ast_new_id($1, CCMMC_KIND_ID_NORMAL), $3);
                }
            | DL_SEMICOL
                {
                    $$ = ccmmc_ast_new(CCMMC_AST_NODE_NUL);
                }
            | RETURN DL_SEMICOL
                {
                    $$ = ccmmc_ast_new_stmt(CCMMC_KIND_STMT_RETURN);
                }
            | RETURN relop_expr DL_SEMICOL
                {
                    $$ = ccmmc_ast_new_stmt(CCMMC_KIND_STMT_RETURN);
                    ccmmc_ast_append_child($$, $2);
                }
            ;

assign_expr_list: nonempty_assign_expr_list
                    {
                        $$ = ccmmc_ast_new(
                            CCMMC_AST_NODE_NONEMPTY_ASSIGN_EXPR_LIST);
                        ccmmc_ast_append_child($$, $1);
                    }
                |
                    {
                        $$ = ccmmc_ast_new(CCMMC_AST_NODE_NUL);
                    }
                ;

nonempty_assign_expr_list   : nonempty_assign_expr_list DL_COMMA assign_expr
                               {
                                   $$ = ccmmc_ast_append_sibling($1, $3);
                               }
                            | assign_expr
                               {
                                   $$ = $1;
                               }
                            ;

assign_expr     : ID OP_ASSIGN relop_expr
                    {
                        $$ = ccmmc_ast_new_stmt(CCMMC_KIND_STMT_ASSIGN);
                        ccmmc_ast_append_children($$, 2,
                            ccmmc_ast_new_id($1, CCMMC_KIND_ID_NORMAL), $3);
                    }
                | relop_expr
                    {
                        $$ = $1;
                    }
                ;

relop_expr      : relop_term
                    {
                        $$ = $1;
                    }
                | relop_expr OP_OR relop_term
                    {
                        $$ = ccmmc_ast_new_expr(
                            CCMMC_KIND_EXPR_BINARY_OP, CCMMC_KIND_OP_BINARY_OR);
                        ccmmc_ast_append_children($$, 2, $1, $3);
                    }
                ;

relop_term      : relop_factor
                    {
                        $$ = $1;
                    }
                | relop_term OP_AND relop_factor
                    {
                        $$ = ccmmc_ast_new_expr(
                            CCMMC_KIND_EXPR_BINARY_OP, CCMMC_KIND_OP_BINARY_AND);
                        ccmmc_ast_append_children($$, 2, $1, $3);
                    }
                ;

relop_factor    : expr
                    {
                        $$ = $1;
                    }
                | expr rel_op expr
                    {
                        $$ = $2;
                        ccmmc_ast_append_children($$, 2, $1, $3);
                    }
                ;

rel_op          : OP_EQ
                    {
                        $$ = ccmmc_ast_new_expr(
                            CCMMC_KIND_EXPR_BINARY_OP, CCMMC_KIND_OP_BINARY_EQ);
                    }
                | OP_GE
                    {
                        $$ = ccmmc_ast_new_expr(
                            CCMMC_KIND_EXPR_BINARY_OP, CCMMC_KIND_OP_BINARY_GE);
                    }
                | OP_LE
                    {
                        $$ = ccmmc_ast_new_expr(
                            CCMMC_KIND_EXPR_BINARY_OP, CCMMC_KIND_OP_BINARY_LE);
                    }
                | OP_NE
                    {
                        $$ = ccmmc_ast_new_expr(
                            CCMMC_KIND_EXPR_BINARY_OP, CCMMC_KIND_OP_BINARY_NE);
                    }
                | OP_GT
                    {
                        $$ = ccmmc_ast_new_expr(
                            CCMMC_KIND_EXPR_BINARY_OP, CCMMC_KIND_OP_BINARY_GT);
                    }
                | OP_LT
                    {
                        $$ = ccmmc_ast_new_expr(
                            CCMMC_KIND_EXPR_BINARY_OP, CCMMC_KIND_OP_BINARY_LT);
                    }
                ;


relop_expr_list : nonempty_relop_expr_list
                    {
                        $$ = ccmmc_ast_new(
                            CCMMC_AST_NODE_NONEMPTY_RELOP_EXPR_LIST);
                        ccmmc_ast_append_child($$, $1);
                    }
                |
                    {
                        $$ = ccmmc_ast_new(CCMMC_AST_NODE_NUL);
                    }
                ;

nonempty_relop_expr_list: nonempty_relop_expr_list DL_COMMA relop_expr
                            {
                                $$ = ccmmc_ast_append_sibling($1, $3);
                            }
                        | relop_expr
                            {
                                $$ = $1;
                            }
                        ;

expr        : expr add_op term
                {
                    $$ = $2;
                    ccmmc_ast_append_children($$, 2, $1, $3);
                }
            | term
                {
                    $$ = $1;
                }
            ;

add_op      : OP_ADD
                {
                    $$ = ccmmc_ast_new_expr(
                        CCMMC_KIND_EXPR_BINARY_OP, CCMMC_KIND_OP_BINARY_ADD);
                }
            | OP_SUB
                {
                    $$ = ccmmc_ast_new_expr(
                        CCMMC_KIND_EXPR_BINARY_OP, CCMMC_KIND_OP_BINARY_SUB);
                }
            ;

term        : term mul_op factor
                {
                    $$ = $2;
                    ccmmc_ast_append_children($$, 2, $1, $3);
                }
            | factor
                {
                    $$ = $1;
                }
            ;

mul_op      : OP_MUL
                {
                    $$ = ccmmc_ast_new_expr(
                        CCMMC_KIND_EXPR_BINARY_OP, CCMMC_KIND_OP_BINARY_MUL);
                }
            | OP_DIV
                {
                    $$ = ccmmc_ast_new_expr(
                        CCMMC_KIND_EXPR_BINARY_OP, CCMMC_KIND_OP_BINARY_DIV);
                }
            ;

factor      : DL_LPAREN relop_expr DL_RPAREN
                {
                    $$ = $2;
                }
            | OP_ADD DL_LPAREN relop_expr DL_RPAREN
                {
                    $$ = ccmmc_ast_new_expr(
                        CCMMC_KIND_EXPR_UNARY_OP, CCMMC_KIND_OP_UNARY_POSITIVE);
                    ccmmc_ast_append_child($$, $3);
                }
            | OP_SUB DL_LPAREN relop_expr DL_RPAREN
                {
                    $$ = ccmmc_ast_new_expr(
                        CCMMC_KIND_EXPR_UNARY_OP, CCMMC_KIND_OP_UNARY_NEGATIVE);
                    ccmmc_ast_append_child($$, $3);
                }
            | OP_NOT DL_LPAREN relop_expr DL_RPAREN
                {
                    $$ = ccmmc_ast_new_expr(
                        CCMMC_KIND_EXPR_UNARY_OP, CCMMC_KIND_OP_UNARY_LOGICAL_NEGATION);
                    ccmmc_ast_append_child($$, $3);
                }
            | CONST
                {
                    $$ = ccmmc_ast_new(CCMMC_AST_NODE_CONST_VALUE);
                    $$->value_const = $1;
                }
            | OP_ADD CONST
                {
                    $$ = ccmmc_ast_new_expr(
                        CCMMC_KIND_EXPR_UNARY_OP, CCMMC_KIND_OP_UNARY_POSITIVE);
                    CcmmcAst *const_node = ccmmc_ast_new(CCMMC_AST_NODE_CONST_VALUE);
                    const_node->value_const = $2;
                    ccmmc_ast_append_child($$, const_node);
                }
            | OP_SUB CONST
                {
                    $$ = ccmmc_ast_new_expr(
                        CCMMC_KIND_EXPR_UNARY_OP, CCMMC_KIND_OP_UNARY_NEGATIVE);
                    CcmmcAst *const_node = ccmmc_ast_new(CCMMC_AST_NODE_CONST_VALUE);
                    const_node->value_const = $2;
                    ccmmc_ast_append_child($$, const_node);
                }
            | OP_NOT CONST
                {
                    $$ = ccmmc_ast_new_expr(
                        CCMMC_KIND_EXPR_UNARY_OP, CCMMC_KIND_OP_UNARY_LOGICAL_NEGATION);
                    CcmmcAst *const_node = ccmmc_ast_new(CCMMC_AST_NODE_CONST_VALUE);
                    const_node->value_const = $2;
                    ccmmc_ast_append_child($$, const_node);
                }
            | ID DL_LPAREN relop_expr_list DL_RPAREN
                {
                    $$ = ccmmc_ast_new_stmt(
                        CCMMC_KIND_STMT_FUNCTION_CALL);
                    ccmmc_ast_append_children($$, 2,
                        ccmmc_ast_new_id($1, CCMMC_KIND_ID_NORMAL), $3);
                }
            | OP_ADD ID DL_LPAREN relop_expr_list DL_RPAREN
                {
                    $$ = ccmmc_ast_new_expr(
                        CCMMC_KIND_EXPR_UNARY_OP, CCMMC_KIND_OP_UNARY_POSITIVE);
                    CcmmcAst *func_node = ccmmc_ast_new_stmt(
                        CCMMC_KIND_STMT_FUNCTION_CALL);
                    ccmmc_ast_append_children(func_node, 2,
                        ccmmc_ast_new_id($2, CCMMC_KIND_ID_NORMAL), $4);
                    ccmmc_ast_append_child($$, func_node);
                }
            | OP_SUB ID DL_LPAREN relop_expr_list DL_RPAREN
                {
                    $$ = ccmmc_ast_new_expr(
                        CCMMC_KIND_EXPR_UNARY_OP, CCMMC_KIND_OP_UNARY_NEGATIVE);
                    CcmmcAst *func_node = ccmmc_ast_new_stmt(
                        CCMMC_KIND_STMT_FUNCTION_CALL);
                    ccmmc_ast_append_children(func_node, 2,
                        ccmmc_ast_new_id($2, CCMMC_KIND_ID_NORMAL), $4);
                    ccmmc_ast_append_child($$, func_node);
                }
            | OP_NOT ID DL_LPAREN relop_expr_list DL_RPAREN
                {
                    $$ = ccmmc_ast_new_expr(
                        CCMMC_KIND_EXPR_UNARY_OP, CCMMC_KIND_OP_UNARY_LOGICAL_NEGATION);
                    CcmmcAst *func_node = ccmmc_ast_new_stmt(
                        CCMMC_KIND_STMT_FUNCTION_CALL);
                    ccmmc_ast_append_children(func_node, 2,
                        ccmmc_ast_new_id($2, CCMMC_KIND_ID_NORMAL), $4);
                    ccmmc_ast_append_child($$, func_node);
                }
            | var_ref
                {
                    $$ = $1;
                }
            | OP_ADD var_ref
                {
                    $$ = ccmmc_ast_new_expr(
                        CCMMC_KIND_EXPR_UNARY_OP, CCMMC_KIND_OP_UNARY_POSITIVE);
                    ccmmc_ast_append_child($$, $2);
                }
            | OP_SUB var_ref
                {
                    $$ = ccmmc_ast_new_expr(
                        CCMMC_KIND_EXPR_UNARY_OP, CCMMC_KIND_OP_UNARY_NEGATIVE);
                    ccmmc_ast_append_child($$, $2);
                }
            | OP_NOT var_ref
                {
                    $$ = ccmmc_ast_new_expr(
                        CCMMC_KIND_EXPR_UNARY_OP, CCMMC_KIND_OP_UNARY_LOGICAL_NEGATION);
                    ccmmc_ast_append_child($$, $2);
                }
            ;

var_ref     : ID
                {
                    $$ = ccmmc_ast_new_id($1, CCMMC_KIND_ID_NORMAL);
                }
            | ID dim_list
                {
                    $$ = ccmmc_ast_new_id($1, CCMMC_KIND_ID_ARRAY);
                    ccmmc_ast_append_child($$, $2);
                }
            ;


dim_list    : dim_list DL_LBRACK expr DL_RBRACK
                {
                    $$ = ccmmc_ast_append_sibling($1, $3);
                }
            | DL_LBRACK expr DL_RBRACK
                {
                    $$ = $2;
                }
            ;

%%

static void ccmmc_parser_error(yyscan_t scanner, CcmmcState *state, const char *mesg)
{
    fprintf(stderr, "Error found in Line \t%zu\tnext token: \t%s\n",
        state->line_number, ccmmc_parser_get_text(scanner));
    exit(1);
}

// vim: set sw=4 ts=4 sts=4 et:
