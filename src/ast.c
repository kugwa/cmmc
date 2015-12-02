#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "ast.h"
#include "common.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


CcmmcAst *ccmmc_ast_new(CcmmcAstNodeType type_node)
{
    CcmmcAst *node;
    node = malloc(sizeof(CcmmcAst));
    ERR_FATAL_CHECK(node, malloc);
    node->parent = NULL;
    node->child = NULL;
    // Notice that leftmostSibling is not initialized as NULL
    node->leftmost_sibling = node;
    node->right_sibling = NULL;
    node->type_node = type_node;
    node->type_value = CCMMC_AST_VALUE_NONE;
    return node;
}

CcmmcAst *ccmmc_ast_append_sibling(CcmmcAst *node, CcmmcAst *sibling)
{
    while (node->right_sibling)
        node = node->right_sibling;
    if (sibling == NULL)
        return node;
    sibling = sibling->leftmost_sibling;
    node->right_sibling = sibling;

    sibling->leftmost_sibling = node->leftmost_sibling;
    sibling->parent = node->parent;
    while (sibling->right_sibling) {
        sibling = sibling->right_sibling;
        sibling->leftmost_sibling = node->leftmost_sibling;
        sibling->parent = node->parent;
    }
    return sibling;
}

CcmmcAst *ccmmc_ast_append_child(CcmmcAst *parent, CcmmcAst *child)
{
    if (child == NULL)
        return parent;
    if (parent->child) {
        ccmmc_ast_append_sibling(parent->child, child);
    } else {
        child = child->leftmost_sibling;
        parent->child = child;
        while (child) {
            child->parent = parent;
            child = child->right_sibling;
        }
    }
    return parent;
}

CcmmcAst *ccmmc_ast_append_children(
    CcmmcAst *parent, size_t children_count, ...)
{
    va_list children_list;
    va_start(children_list, children_count);
    CcmmcAst *child = va_arg(children_list, CcmmcAst*);
    ccmmc_ast_append_child(parent, child);
    CcmmcAst *tmp = child;
    for (size_t index = 1; index < children_count; ++index) {
        child = va_arg(children_list, CcmmcAst*);
        tmp = ccmmc_ast_append_sibling(tmp, child);
    }
    va_end(children_list);
    return parent;
}

CcmmcAst *ccmmc_ast_new_id(char *lexeme, CcmmcKindId kind)
{
    CcmmcAst *node = ccmmc_ast_new(CCMMC_AST_NODE_ID);
    node->value_id.kind = kind;
    node->value_id.name = lexeme;
    // node->value_id.symbolTableEntry = NULL;
    return node;
}

CcmmcAst *ccmmc_ast_new_stmt(CcmmcKindStmt kind)
{
    CcmmcAst *node = ccmmc_ast_new(CCMMC_AST_NODE_STMT);
    node->value_stmt.kind = kind;
    return node;
}

CcmmcAst *ccmmc_ast_new_decl(CcmmcKindDecl kind)
{
    CcmmcAst *node = ccmmc_ast_new(CCMMC_AST_NODE_DECL);
    node->value_decl.kind = kind;
    return node;
}

CcmmcAst *ccmmc_ast_new_expr(CcmmcKindExpr kind, int op_kind)
{
    CcmmcAst *node = ccmmc_ast_new(CCMMC_AST_NODE_EXPR);
    node->value_expr.kind = kind;
    node->value_expr.is_const_eval = false;
    if (kind == CCMMC_KIND_EXPR_BINARY_OP)
        node->value_expr.op_binary = op_kind;
    else if (kind == CCMMC_KIND_EXPR_UNARY_OP)
        node->value_expr.op_unary = op_kind;
    else
        fprintf(stderr, "%s: invalid expression kind in %s\n", prog_name, __func__);
    return node;
}

// vim: set sw=4 ts=4 sts=4 et:
