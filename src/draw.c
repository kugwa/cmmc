#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "draw.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


static void printLabelString(FILE *fp, CcmmcAst *astNode)
{
    const char *binaryOpString[] = {
        "+",
        "-",
        "*",
        "/",
        "==",
        ">=",
        "<=",
        "!=",
        ">",
        "<",
        "&&",
        "||"
    };
    const char *unaryOpString[] = {
        "+",
        "-",
        "!"
    };
    switch (astNode->type_node) {
        case CCMMC_AST_NODE_PROGRAM:
            fprintf(fp, "PROGRAM_NODE");
            break;
        case CCMMC_AST_NODE_DECL:
            fprintf(fp, "DECLARATION_NODE ");
            switch (astNode->value_decl.kind) {
                case CCMMC_KIND_DECL_VARIABLE:
                    fprintf(fp, "VARIABLE_DECL");
                    break;
                case CCMMC_KIND_DECL_TYPE:
                    fprintf(fp, "TYPE_DECL");
                    break;
                case CCMMC_KIND_DECL_FUNCTION:
                    fprintf(fp, "FUNCTION_DECL");
                    break;
                case CCMMC_KIND_DECL_FUNCTION_PARAMETER:
                    fprintf(fp, "FUNCTION_PARAMETER_DECL");
                    break;
                default:
                    assert(false);
            }
            break;
        case CCMMC_AST_NODE_ID:
            fprintf(fp, "IDENTIFIER_NODE ");
            fprintf(fp, "%s ", astNode->value_id.name);
            switch (astNode->value_id.kind) {
                case CCMMC_KIND_ID_NORMAL:
                    fprintf(fp, "NORMAL_ID");
                    break;
                case CCMMC_KIND_ID_ARRAY:
                    fprintf(fp, "ARRAY_ID");
                    break;
                case CCMMC_KIND_ID_WITH_INIT:
                    fprintf(fp, "WITH_INIT_ID");
                    break;
                default:
                    assert(false);
            }
            break;
        case CCMMC_AST_NODE_PARAM_LIST:
            fprintf(fp, "PARAM_LIST_NODE");
            break;
        case CCMMC_AST_NODE_NUL:
            fprintf(fp, "NUL_NODE");
            break;
        case CCMMC_AST_NODE_BLOCK:
            fprintf(fp, "BLOCK_NODE");
            break;
        case CCMMC_AST_NODE_VARIABLE_DECL_LIST:
            fprintf(fp, "VARIABLE_DECL_LIST_NODE");
            break;
        case CCMMC_AST_NODE_STMT_LIST:
            fprintf(fp, "STMT_LIST_NODE");
            break;
        case CCMMC_AST_NODE_STMT:
            fprintf(fp, "STMT_NODE ");
            switch (astNode->value_stmt.kind) {
                case CCMMC_KIND_STMT_WHILE:
                    fprintf(fp, "WHILE_STMT");
                    break;
                case CCMMC_KIND_STMT_FOR:
                    fprintf(fp, "FOR_STMT");
                    break;
                case CCMMC_KIND_STMT_ASSIGN:
                    fprintf(fp, "ASSIGN_STMT");
                    break;
                case CCMMC_KIND_STMT_IF:
                    fprintf(fp, "IF_STMT");
                    break;
                case CCMMC_KIND_STMT_FUNCTION_CALL:
                    fprintf(fp, "FUNCTION_CALL_STMT");
                    break;
                case CCMMC_KIND_STMT_RETURN:
                    fprintf(fp, "RETURN_STMT");
                    break;
                default:
                    assert(false);
            }
            break;
        case CCMMC_AST_NODE_EXPR:
            fprintf(fp, "EXPR_NODE ");
            switch (astNode->value_expr.kind) {
                case CCMMC_KIND_EXPR_BINARY_OP:
                    fprintf(fp, "%s", binaryOpString[astNode->value_expr.op_binary]);
                    break;
                case CCMMC_KIND_EXPR_UNARY_OP:
                    fprintf(fp, "%s", unaryOpString[astNode->value_expr.op_unary]);
                    break;
                default:
                    assert(false);
            }
            break;
        case CCMMC_AST_NODE_CONST_VALUE:
            fprintf(fp, "CONST_VALUE_NODE ");
            switch (astNode->value_const.kind) {
                case CCMMC_KIND_CONST_INT:
                    fprintf(fp, "%d", astNode->value_const.const_int);
                    break;
                case CCMMC_KIND_CONST_FLOAT:
                    fprintf(fp, "%f", astNode->value_const.const_float);
                    break;
                case CCMMC_KIND_CONST_STRING:
                    fprintf(fp, "\\\"%s\\\"", astNode->value_const.const_string);
                    break;
                default:
                    assert(false);
            }
            break;
        case CCMMC_AST_NODE_NONEMPTY_ASSIGN_EXPR_LIST:
            fprintf(fp, "NONEMPTY_ASSIGN_EXPR_LIST_NODE");
            break;
        case CCMMC_AST_NODE_NONEMPTY_RELOP_EXPR_LIST:
            fprintf(fp, "NONEMPTY_RELOP_EXPR_LIST_NODE");
            break;
        default:
            fprintf(fp, "default case in char *getLabelString(AST_TYPE astType)");
            break;
    }
}

// count: the (unused) id number to be used
// return: then next unused id number
static int printGVNode(FILE *fp, CcmmcAst *node, int count)
{
    if (node == NULL)
        return count;

    int currentNodeCount = count;
    fprintf(fp, "node%d [label =\"", count);
    printLabelString(fp, node);
    fprintf(fp, "\"]\n");
    ++count;
    int countAfterCheckChildren = count;
    if (node->child) {
        countAfterCheckChildren = printGVNode(fp, node->child, count);
        fprintf(fp, "node%d -> node%d [style = bold]\n", currentNodeCount, count);
    }

    int countAfterCheckSibling = countAfterCheckChildren;
    if (node->right_sibling) {
        countAfterCheckSibling = printGVNode(fp, node->right_sibling, countAfterCheckChildren);
        fprintf(fp, "node%d -> node%d [style = dashed]\n", currentNodeCount, countAfterCheckChildren);
    }

    return countAfterCheckSibling;
}

void ccmmc_draw_ast(FILE *fp, const char *name, CcmmcAst *root)
{
    fprintf(fp , "Digraph AST\n");
    fprintf(fp , "{\n");
    fprintf(fp , "label = \"%s\"\n", name);
    printGVNode(fp, root, 0);
    fprintf(fp , "}\n");
}

// vim: set sw=4 ts=4 sts=4 et:
