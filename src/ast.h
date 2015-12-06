#ifndef CCMMC_HEADER_AST_H
#define CCMMC_HEADER_AST_H

#include <stdbool.h>
#include <stddef.h>

typedef enum CcmmcAstNodeType_enum {
    CCMMC_AST_NODE_PROGRAM,
    CCMMC_AST_NODE_DECL,
    CCMMC_AST_NODE_ID,
    CCMMC_AST_NODE_PARAM_LIST,
    CCMMC_AST_NODE_NUL,
    CCMMC_AST_NODE_BLOCK,
    CCMMC_AST_NODE_VARIABLE_DECL_LIST,
    CCMMC_AST_NODE_STMT_LIST,
    CCMMC_AST_NODE_STMT,
    CCMMC_AST_NODE_EXPR,
    CCMMC_AST_NODE_CONST_VALUE, // ex: 1, 2, "constant string"
    CCMMC_AST_NODE_NONEMPTY_ASSIGN_EXPR_LIST,
    CCMMC_AST_NODE_NONEMPTY_RELOP_EXPR_LIST
} CcmmcAstNodeType;

typedef enum CcmmcAstValueType_enum {
    CCMMC_AST_VALUE_INT,
    CCMMC_AST_VALUE_FLOAT,
    CCMMC_AST_VALUE_VOID,
    CCMMC_AST_VALUE_INT_PTR, // for parameter passing
    CCMMC_AST_VALUE_FLOAT_PTR, // for parameter passing
    CCMMC_AST_VALUE_CONST_STRING, // for "const string"
    CCMMC_AST_VALUE_NONE, // for nodes like PROGRAM_NODE which has no type
    CCMMC_AST_VALUE_ERROR
} CcmmcAstValueType;

typedef enum CcmmcKindId_enum {
    CCMMC_KIND_ID_NORMAL, // function names, uninitialized scalar variables
    CCMMC_KIND_ID_ARRAY, // ID_NODE->child = dim
    CCMMC_KIND_ID_WITH_INIT, // ID_NODE->child = initializer
} CcmmcKindId;

typedef enum CcmmcKindOpBinary_enum {
    CCMMC_KIND_OP_BINARY_ADD,
    CCMMC_KIND_OP_BINARY_SUB,
    CCMMC_KIND_OP_BINARY_MUL,
    CCMMC_KIND_OP_BINARY_DIV,
    CCMMC_KIND_OP_BINARY_EQ,
    CCMMC_KIND_OP_BINARY_GE,
    CCMMC_KIND_OP_BINARY_LE,
    CCMMC_KIND_OP_BINARY_NE,
    CCMMC_KIND_OP_BINARY_GT,
    CCMMC_KIND_OP_BINARY_LT,
    CCMMC_KIND_OP_BINARY_AND,
    CCMMC_KIND_OP_BINARY_OR
} CcmmcKindOpBinary;

typedef enum CcmmcKindOpUnary_enum {
    CCMMC_KIND_OP_UNARY_POSITIVE,
    CCMMC_KIND_OP_UNARY_NEGATIVE,
    CCMMC_KIND_OP_UNARY_LOGICAL_NEGATION
} CcmmcKindOpUnary;

typedef enum CcmmcKindConst_enum {
    CCMMC_KIND_CONST_INT,
    CCMMC_KIND_CONST_FLOAT,
    CCMMC_KIND_CONST_STRING
} CcmmcKindConst;

typedef enum CcmmcKindStmt_enum {
    CCMMC_KIND_STMT_WHILE,
    CCMMC_KIND_STMT_FOR,
    CCMMC_KIND_STMT_ASSIGN, // TODO: for simpler implementation, assign_expr also uses this
    CCMMC_KIND_STMT_IF,
    CCMMC_KIND_STMT_FUNCTION_CALL,
    CCMMC_KIND_STMT_RETURN,
} CcmmcKindStmt;

typedef enum CcmmcKindExpr_enum {
    CCMMC_KIND_EXPR_BINARY_OP,
    CCMMC_KIND_EXPR_UNARY_OP
} CcmmcKindExpr;

typedef enum CcmmcKindDecl_enum {
    CCMMC_KIND_DECL_VARIABLE,
    CCMMC_KIND_DECL_TYPE,
    CCMMC_KIND_DECL_FUNCTION,
    CCMMC_KIND_DECL_FUNCTION_PARAMETER
} CcmmcKindDecl;

typedef struct CcmmcValueStmt_struct {
    CcmmcKindStmt kind;
} CcmmcValueStmt;

typedef struct CcmmcValueExpr_struct {
    CcmmcKindExpr kind;
    bool is_const_eval;
    union {
        int const_int;
        float const_float;
    };
    union {
        CcmmcKindOpBinary op_binary;
        CcmmcKindOpUnary op_unary;
    };
} CcmmcValueExpr;

typedef struct CcmmcValueDecl_struct {
    CcmmcKindDecl kind;
} CcmmcValueDecl;

typedef struct CcmmcValueId_struct {
    CcmmcKindId kind;
    char *name;
    // struct SymbolTableEntry *symbolTableEntry;
} CcmmcValueId;

typedef struct CcmmcValueType_struct {
    char *name;
} CcmmcValueType;

typedef struct CcmmcValueConst_struct {
    CcmmcKindConst kind;
    union {
        int const_int;
        float const_float;
        char *const_string;
    };
} CcmmcValueConst;

typedef struct CcmmcAst_struct {
    struct CcmmcAst_struct *parent;
    struct CcmmcAst_struct *child;
    struct CcmmcAst_struct *leftmost_sibling;
    struct CcmmcAst_struct *right_sibling;
    CcmmcAstNodeType type_node;
    CcmmcAstValueType type_value;
    union {
        CcmmcValueId value_id;
        CcmmcValueStmt value_stmt;
        CcmmcValueDecl value_decl;
        CcmmcValueExpr value_expr;
        CcmmcValueConst value_const;
    };
} CcmmcAst;

CcmmcAst        *ccmmc_ast_new                      (CcmmcAstNodeType type_node);
CcmmcAst        *ccmmc_ast_new_id                   (char *lexeme,
                                                     CcmmcKindId kind);
CcmmcAst        *ccmmc_ast_new_stmt                 (CcmmcKindStmt kind);
CcmmcAst        *ccmmc_ast_new_decl                 (CcmmcKindDecl kind);
CcmmcAst        *ccmmc_ast_new_expr                 (CcmmcKindExpr kind,
                                                     int op_kind);
CcmmcAst        *ccmmc_ast_append_sibling           (CcmmcAst *node,
                                                     CcmmcAst *sibling);
CcmmcAst        *ccmmc_ast_append_child             (CcmmcAst *parent,
                                                     CcmmcAst *child);
CcmmcAst        *ccmmc_ast_append_children          (CcmmcAst *parent,
                                                     size_t children_count,
                                                     ...);

#endif
// vim: set sw=4 ts=4 sts=4 et:
