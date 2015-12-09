#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "semantic-analysis.h"
#include "common.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERROR(msg) ("Error found in line %zu\n" msg "\n")


static CcmmcValueConst eval_const_expr(CcmmcAst *expr) {
    if (expr->type_node == CCMMC_AST_NODE_CONST_VALUE) {
        if (expr->value_const.kind == CCMMC_KIND_CONST_STRING) {
            fprintf(stderr, ERROR("Strings are not allowed in expressions."),
                expr->line_number);
            return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_ERROR };
        }
        return expr->value_const;
    }
    if (expr->type_node != CCMMC_AST_NODE_EXPR) {
        fprintf(stderr, ERROR("Not a constant expression."), expr->line_number);
        return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_ERROR };
    }

#define EVAL_AND_RETURN_BINARY_ARITH(op) \
    do { \
        if (left.kind == CCMMC_KIND_CONST_INT) { \
            if (right.kind == CCMMC_KIND_CONST_INT) { \
                int result = left.const_int op right.const_int; \
                ccmmc_ast_expr_set_is_constant(expr, true); \
                ccmmc_ast_expr_set_is_int(expr); \
                ccmmc_ast_expr_set_int(expr, result); \
                return (CcmmcValueConst){ \
                    .kind = CCMMC_KIND_CONST_INT, .const_int = result }; \
            } else if (right.kind == CCMMC_KIND_CONST_FLOAT) { \
                float left_value = left.const_int; \
                float right_value = right.const_float; \
                float result = left_value op right_value; \
                ccmmc_ast_expr_set_is_constant(expr, true); \
                ccmmc_ast_expr_set_is_float(expr); \
                ccmmc_ast_expr_set_float(expr, result); \
                return (CcmmcValueConst){ \
                    .kind = CCMMC_KIND_CONST_FLOAT, .const_float = result }; \
            } \
            assert(false); \
        } else if (left.kind == CCMMC_KIND_CONST_FLOAT) { \
            if (right.kind == CCMMC_KIND_CONST_INT) { \
                float left_value = left.const_float; \
                float right_value = right.const_int; \
                float result = left_value op right_value; \
                ccmmc_ast_expr_set_is_constant(expr, true); \
                ccmmc_ast_expr_set_is_float(expr); \
                ccmmc_ast_expr_set_float(expr, result); \
                return (CcmmcValueConst){ \
                    .kind = CCMMC_KIND_CONST_FLOAT, .const_float = result }; \
            } else if (right.kind == CCMMC_KIND_CONST_FLOAT) { \
                float result = left.const_float op right.const_float; \
                ccmmc_ast_expr_set_is_constant(expr, true); \
                ccmmc_ast_expr_set_is_float(expr); \
                ccmmc_ast_expr_set_float(expr, result); \
                return (CcmmcValueConst){ \
                    .kind = CCMMC_KIND_CONST_FLOAT, .const_float = result }; \
            } \
            assert(false); \
        } \
        assert(false); \
    } while (false)

#define EVAL_AND_RETURN_BINARY_REL(op) \
    do { \
        ccmmc_ast_expr_set_is_constant(expr, true); \
        ccmmc_ast_expr_set_is_int(expr); \
        int result; \
        if (left.kind == CCMMC_KIND_CONST_INT) { \
            if (right.kind == CCMMC_KIND_CONST_INT) { \
                result = left.const_int op right.const_int; \
            } else if (right.kind == CCMMC_KIND_CONST_FLOAT) { \
                float left_value = left.const_int; \
                float right_value = right.const_float; \
                result = left_value op right_value; \
            } else { \
                assert(false); \
            } \
        } else if (left.kind == CCMMC_KIND_CONST_FLOAT) { \
            if (right.kind == CCMMC_KIND_CONST_INT) { \
                float left_value = left.const_float; \
                float right_value = right.const_int; \
                result = left_value op right_value; \
            } else if (right.kind == CCMMC_KIND_CONST_FLOAT) { \
                result = left.const_float op right.const_float; \
            } else { \
                assert(false); \
            } \
        } else { \
            assert(false); \
        } \
        ccmmc_ast_expr_set_int(expr, result); \
        return (CcmmcValueConst){ \
            .kind = CCMMC_KIND_CONST_INT, .const_int = result }; \
    } while (false)

#define EVAL_AND_RETURN_UNARY_SIGN(op) \
    do { \
        if (arg.kind == CCMMC_KIND_CONST_INT) { \
            int result = op arg.const_int; \
            ccmmc_ast_expr_set_is_constant(expr, true); \
            ccmmc_ast_expr_set_is_int(expr); \
            ccmmc_ast_expr_set_int(expr, result); \
            return (CcmmcValueConst){ \
                .kind = CCMMC_KIND_CONST_INT, .const_int = result }; \
        } else if (arg.kind == CCMMC_KIND_CONST_FLOAT) { \
            float result = op arg.const_float; \
            ccmmc_ast_expr_set_is_constant(expr, true); \
            ccmmc_ast_expr_set_is_float(expr); \
            ccmmc_ast_expr_set_float(expr, result); \
            return (CcmmcValueConst){ \
                .kind = CCMMC_KIND_CONST_FLOAT, .const_float = result }; \
        } \
        assert(false); \
    } while (false)

    if (expr->value_expr.kind == CCMMC_KIND_EXPR_BINARY_OP) {
        CcmmcValueConst left = eval_const_expr(expr->child);
        CcmmcValueConst right = eval_const_expr(expr->child->right_sibling);
        if (left.kind == CCMMC_KIND_CONST_ERROR ||
            right.kind == CCMMC_KIND_CONST_ERROR)
            return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_ERROR };

        switch (expr->value_expr.op_binary) {
            case CCMMC_KIND_OP_BINARY_ADD:
                EVAL_AND_RETURN_BINARY_ARITH(+);
            case CCMMC_KIND_OP_BINARY_SUB:
                EVAL_AND_RETURN_BINARY_ARITH(-);
            case CCMMC_KIND_OP_BINARY_MUL:
                EVAL_AND_RETURN_BINARY_ARITH(*);
            case CCMMC_KIND_OP_BINARY_DIV:
                if (left.kind == CCMMC_KIND_CONST_INT &&
                    right.kind == CCMMC_KIND_CONST_INT &&
                    right.const_int == 0) {
                    fprintf(stderr, ERROR("Integer division by zero."),
                        expr->line_number);
                    return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_ERROR };
                }
                EVAL_AND_RETURN_BINARY_ARITH(/);
            case CCMMC_KIND_OP_BINARY_EQ:
                EVAL_AND_RETURN_BINARY_REL(==);
            case CCMMC_KIND_OP_BINARY_GE:
                EVAL_AND_RETURN_BINARY_REL(>=);
            case CCMMC_KIND_OP_BINARY_LE:
                EVAL_AND_RETURN_BINARY_REL(<=);
            case CCMMC_KIND_OP_BINARY_NE:
                EVAL_AND_RETURN_BINARY_REL(!=);
            case CCMMC_KIND_OP_BINARY_GT:
                EVAL_AND_RETURN_BINARY_REL(>);
            case CCMMC_KIND_OP_BINARY_LT:
                EVAL_AND_RETURN_BINARY_REL(<);
            case CCMMC_KIND_OP_BINARY_AND:
                EVAL_AND_RETURN_BINARY_REL(&&);
            case CCMMC_KIND_OP_BINARY_OR:
                EVAL_AND_RETURN_BINARY_REL(||);
            default:
                assert(false);
        }
    } else if (expr->value_expr.kind == CCMMC_KIND_EXPR_UNARY_OP) {
        CcmmcValueConst arg = eval_const_expr(expr->child);
        if (arg.kind == CCMMC_KIND_CONST_ERROR)
            return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_ERROR };

        switch (expr->value_expr.op_unary) {
            case CCMMC_KIND_OP_UNARY_POSITIVE:
                EVAL_AND_RETURN_UNARY_SIGN(+);
            case CCMMC_KIND_OP_UNARY_NEGATIVE:
                EVAL_AND_RETURN_UNARY_SIGN(-);
            case CCMMC_KIND_OP_UNARY_LOGICAL_NEGATION: {
                int result;
                if (arg.kind == CCMMC_KIND_CONST_INT) {
                    result = !arg.const_int;
                } else if (arg.kind == CCMMC_KIND_CONST_FLOAT) {
                    result = !arg.const_float;
                } else {
                    assert(false);
                }
                ccmmc_ast_expr_set_is_constant(expr, true);
                ccmmc_ast_expr_set_is_int(expr);
                ccmmc_ast_expr_set_int(expr, result);
                return (CcmmcValueConst){
                    .kind = CCMMC_KIND_CONST_INT, .const_int = result };
                }
            default:
                assert(false);
        }
    } else {
        assert(false);
    }
#undef EVAL_AND_RETURN_BINARY_ARITH
#undef EVAL_AND_RETURN_BINARY_REL
#undef EVAL_AND_RETURN_UNARY_SIGN
}

static size_t *get_array_size(CcmmcAst *id_array, size_t *array_dimension)
{
    CcmmcAst *dim;
    size_t dim_count = 0;
    for (dim = id_array->child; dim != NULL; dim = dim->right_sibling, dim_count++);
    assert(dim_count != 0);

    size_t dim_index = 0;
    size_t *array_size = malloc(sizeof(size_t) * dim_count);
    ERR_FATAL_CHECK(array_size, malloc);
    for (dim = id_array->child; dim != NULL; dim = dim->right_sibling, dim_index++) {
        if (dim->type_node == CCMMC_AST_NODE_NUL)
            array_size[dim_index] = 0;
        else {
            CcmmcValueConst value = eval_const_expr(dim);
            if (value.kind == CCMMC_KIND_CONST_ERROR) {
                free(array_size);
                return NULL;
            }
            if (value.kind != CCMMC_KIND_CONST_INT) {
                fprintf(stderr, ERROR("Array subscript is not an integer."),
                    dim->line_number);
                free(array_size);
                return NULL;
            }
            if (value.const_int <= 0) {
                fprintf(stderr, ERROR("Array size must be positive."),
                    dim->line_number);
                free(array_size);
                return NULL;
            }
            array_size[dim_index] = value.const_int;
        }
    }

    *array_dimension = dim_count;
    return array_size;
}

static size_t *get_array_of_array_size(CcmmcAst *id_array, size_t *array_dimension,
    size_t base_array_dimension, size_t *base_array_size)
{
    size_t this_array_dimension;
    size_t *this_array_size = get_array_size(id_array, &this_array_dimension);
    if (this_array_size == NULL) {
        free (this_array_size);
        return NULL;
    }

    size_t dim_count = this_array_dimension + base_array_dimension;
    size_t *array_size = realloc(this_array_size, sizeof(size_t) * dim_count);
    ERR_FATAL_CHECK(array_size, realloc);
    memcpy(array_size + this_array_dimension,
        base_array_size, sizeof(size_t) * base_array_dimension);

    *array_dimension = dim_count;
    return array_size;
}

static bool decl_typedef(CcmmcAst *type_decl, CcmmcSymbolTable *table)
{
    bool any_error = false;

    // The leftmost child is an existing type
    assert(type_decl->child != NULL);
    assert(type_decl->child->type_node == CCMMC_AST_NODE_ID);
    assert(type_decl->child->value_id.kind == CCMMC_KIND_ID_NORMAL);
    const char *source_str = type_decl->child->value_id.name;
    CcmmcSymbol *source_sym = ccmmc_symbol_table_retrieve(table, source_str);
    // We don't support function pointers
    assert(!ccmmc_symbol_is_function(source_sym));
    if (source_sym == NULL) {
        fprintf(stderr, ERROR("ID `%s' undeclared."),
            type_decl->line_number, source_str);
        return true;
    }
    if (source_sym->kind != CCMMC_SYMBOL_KIND_TYPE) {
        fprintf(stderr, ERROR("ID `%s' is not a type."),
            type_decl->line_number, source_str);
        return true;
    }

    // Other children are newly declared types
    for (CcmmcAst *id = type_decl->child->right_sibling; id != NULL;
         id = id->right_sibling) {
        assert(id->type_node == CCMMC_AST_NODE_ID);
        const char *target_str = id->value_id.name;
        if (ccmmc_symbol_scope_exist(table->current, target_str)) {
            any_error = true;
            fprintf (stderr, ERROR("ID `%s' redeclared."),
                id->line_number, target_str);
            continue;
        }
        switch (id->value_id.kind) {
            case CCMMC_KIND_ID_NORMAL:
                ccmmc_symbol_table_insert(table,
                    target_str, CCMMC_SYMBOL_KIND_TYPE, source_sym->type);
                break;
            case CCMMC_KIND_ID_ARRAY: {
                size_t array_dimension;
                size_t *array_size;
                if (source_sym->type.type_base == CCMMC_AST_VALUE_VOID) {
                    any_error = true;
                    fprintf (stderr, ERROR("ID `%s' is an array of voids."),
                        id->line_number, target_str);
                    continue;
                }
                if (ccmmc_symbol_is_array(source_sym))
                    array_size = get_array_of_array_size(
                        id, &array_dimension,
                        source_sym->type.array_dimension,
                        source_sym->type.array_size);
                else
                    array_size = get_array_size(id, &array_dimension);
                if (array_size == NULL) {
                    any_error = true;
                    continue;
                }
                CcmmcSymbolType type = {
                    .type_base = source_sym->type.type_base,
                    .array_dimension = array_dimension,
                    .array_size = array_size };
                ccmmc_symbol_table_insert(table,
                    target_str, CCMMC_SYMBOL_KIND_TYPE, type);
                } break;
            case CCMMC_KIND_ID_WITH_INIT:
            default:
                assert(false);
        }
    }

    return any_error;
}

static bool check_var_ref(CcmmcAst *ref, CcmmcSymbolTable *table)
{
    bool any_error = false;

    if (ccmmc_symbol_table_retrieve(table, ref->value_id.name) == NULL) {
        fprintf(stderr, ERROR("ID `%s' undeclared."),
            ref->line_number, ref->value_id.name);
        return true;
    }
    if (ref->value_id.kind == CCMMC_KIND_ID_ARRAY) {
        
    }
    return any_error;
}

static bool check_relop_expr(CcmmcAst *expr, CcmmcSymbolTable *table)
{
    bool any_error = false;
    return any_error;
}

static bool decl_variable(
    CcmmcAst *var_decl, CcmmcSymbolTable *table, bool constant_only)
{
    bool any_error = false;

    // The leftmost child is the type of the variable declaration list
    assert(var_decl->child != NULL);
    assert(var_decl->child->type_node == CCMMC_AST_NODE_ID);
    assert(var_decl->child->value_id.kind == CCMMC_KIND_ID_NORMAL);
    const char *type_str = var_decl->child->value_id.name;
    CcmmcSymbol *type_sym = ccmmc_symbol_table_retrieve(table, type_str);
    // We don't support function pointers
    assert(!ccmmc_symbol_is_function(type_sym));
    if (type_sym == NULL) {
        fprintf(stderr, ERROR("ID `%s' undeclared."),
            var_decl->line_number, type_str);
        return true;
    }
    if (type_sym->kind != CCMMC_SYMBOL_KIND_TYPE) {
        fprintf(stderr, ERROR("ID `%s' is not a type."),
            var_decl->line_number, type_str);
        return true;
    }
    if (type_sym->type.type_base == CCMMC_AST_VALUE_VOID) {
        fprintf(stderr, ERROR("ID `%s' is a void type."),
            var_decl->line_number, type_str);
        return true;
    }

    // Other children are newly declared variables
    for (CcmmcAst *init_id = var_decl->child->right_sibling; init_id != NULL;
         init_id = init_id->right_sibling) {
        assert(init_id->type_node = CCMMC_AST_NODE_ID);
        const char *var_str = init_id->value_id.name;
        if (ccmmc_symbol_scope_exist(table->current, var_str)) {
            any_error = true;
            fprintf (stderr, ERROR("ID `%s' redeclared."),
                init_id->line_number, var_str);
            continue;
        }
        switch (init_id->value_id.kind) {
            case CCMMC_KIND_ID_NORMAL:
                ccmmc_symbol_table_insert(table,
                    var_str, CCMMC_SYMBOL_KIND_VARIABLE, type_sym->type);
                break;
            case CCMMC_KIND_ID_ARRAY: {
                size_t array_dimension;
                size_t *array_size;
                if (ccmmc_symbol_is_array(type_sym))
                    array_size = get_array_of_array_size(
                        init_id, &array_dimension,
                        type_sym->type.array_dimension,
                        type_sym->type.array_size);
                else
                    array_size = get_array_size (init_id, &array_dimension);
                if (array_size == NULL) {
                    any_error = true;
                    continue;
                }
                CcmmcSymbolType type = {
                    .type_base = type_sym->type.type_base,
                    .array_dimension = array_dimension,
                    .array_size = array_size };
                ccmmc_symbol_table_insert(table,
                    var_str, CCMMC_SYMBOL_KIND_VARIABLE, type);
                } break;
            case CCMMC_KIND_ID_WITH_INIT: {
                assert(ccmmc_symbol_is_scalar(type_sym));
                assert(init_id->child != NULL);
                CcmmcAst *expr = init_id->child;
                if (constant_only) {
                    CcmmcValueConst result = eval_const_expr(expr);
                    switch (result.kind) {
                        case CCMMC_KIND_CONST_INT:
                            if (type_sym->type.type_base == CCMMC_AST_VALUE_FLOAT) {
                                // int -> float
                                if (expr->type_node == CCMMC_AST_NODE_CONST_VALUE) {
                                    expr->value_const = (CcmmcValueConst){
                                        .kind = CCMMC_KIND_CONST_FLOAT,
                                        .const_float = result.const_int };
                                } else if (expr->type_node == CCMMC_AST_NODE_EXPR) {
                                    ccmmc_ast_expr_set_is_constant(expr, true);
                                    ccmmc_ast_expr_set_is_float(expr);
                                    ccmmc_ast_expr_set_float(expr, result.const_int);
                                } else {
                                    assert(false);
                                }
                            }
                            break;
                        case CCMMC_KIND_CONST_FLOAT:
                            if (type_sym->type.type_base == CCMMC_AST_VALUE_INT) {
                                // float -> int
                                if (expr->type_node == CCMMC_AST_NODE_CONST_VALUE) {
                                    expr->value_const = (CcmmcValueConst){
                                        .kind = CCMMC_KIND_CONST_INT,
                                        .const_int = result.const_float };
                                } else if (expr->type_node == CCMMC_AST_NODE_EXPR) {
                                    ccmmc_ast_expr_set_is_constant(expr, true);
                                    ccmmc_ast_expr_set_is_int(expr);
                                    ccmmc_ast_expr_set_int(expr, result.const_float);
                                } else {
                                    assert(false);
                                }
                            }
                            break;
                        case CCMMC_KIND_CONST_ERROR:
                            any_error = true;
                            continue;
                        case CCMMC_KIND_CONST_STRING:
                            // string is already handled in eval_const_expr
                        default:
                            assert(false);
                    }
                } else {
                    if (check_relop_expr(expr, table)) {
                        any_error = true;
                        continue;
                    }
                }
                ccmmc_symbol_table_insert(table,
                    var_str, CCMMC_SYMBOL_KIND_VARIABLE, type_sym->type);
                } break;
            default:
                assert(false);
        }
    }
    return any_error;
}

static bool process_block(CcmmcAst*, CcmmcSymbolTable*);
static bool process_statement(CcmmcAst *stmt, CcmmcSymbolTable *table)
{
    bool any_error = false;

    if (stmt->type_node == CCMMC_AST_NODE_NUL)
        return false;
    if (stmt->type_node == CCMMC_AST_NODE_BLOCK)
        return process_block(stmt, table) || any_error;
    assert(stmt->type_node == CCMMC_AST_NODE_STMT);

    switch(stmt->value_stmt.kind) {
        case CCMMC_KIND_STMT_WHILE:
            any_error = check_relop_expr(stmt->child, table) || any_error;
            any_error = process_statement(stmt->child->right_sibling, table) || any_error;
            break;
        case CCMMC_KIND_STMT_FOR:
        case CCMMC_KIND_STMT_ASSIGN:
            any_error = check_var_ref(stmt->child, table) || any_error;
            any_error = check_relop_expr(stmt->child->right_sibling, table) || any_error;
            break;
        case CCMMC_KIND_STMT_IF:
            any_error = check_relop_expr(stmt->child, table) || any_error;
            any_error = process_statement(stmt->child->right_sibling, table) || any_error;
            break;
        case CCMMC_KIND_STMT_FUNCTION_CALL:
            break;
        case CCMMC_KIND_STMT_RETURN:
            if (stmt->child != NULL)
                any_error = check_relop_expr(stmt->child, table) || any_error;
            break;
        default:
            assert(false);
    }

    return any_error;
}

static bool process_block(CcmmcAst *block, CcmmcSymbolTable *table)
{
    bool any_error = false;

    // Push a new scope for the block
    ccmmc_symbol_table_open_scope(table);
    // Insert builtin types
    ccmmc_symbol_table_insert(table, "int", CCMMC_SYMBOL_KIND_TYPE,
        (CcmmcSymbolType){ .type_base = CCMMC_AST_VALUE_INT });
    ccmmc_symbol_table_insert(table, "float", CCMMC_SYMBOL_KIND_TYPE,
        (CcmmcSymbolType){ .type_base = CCMMC_AST_VALUE_FLOAT });
    ccmmc_symbol_table_insert(table, "void", CCMMC_SYMBOL_KIND_TYPE,
        (CcmmcSymbolType){ .type_base = CCMMC_AST_VALUE_VOID });

    // This is a function block
    if (block->parent->type_node == CCMMC_AST_NODE_DECL &&
            block->parent->value_decl.kind == CCMMC_KIND_DECL_FUNCTION) {
        // Insert the parameters
        CcmmcSymbol *func = ccmmc_symbol_table_retrieve(table,
            block->leftmost_sibling->right_sibling->value_id.name);
        CcmmcAst *param;
        size_t i;
        for (param = block->leftmost_sibling->right_sibling->right_sibling->child,
                i = 0; i < func->type.param_count; param = param->right_sibling, i++) {
            if (ccmmc_symbol_scope_exist(table->current,
                    param->child->right_sibling->value_id.name)) {
                any_error = true;
                fprintf (stderr, ERROR("ID `%s' redeclared."),
                    param->child->right_sibling->line_number,
                    param->child->right_sibling->value_id.name);
                continue;
            }
            ccmmc_symbol_table_insert(table, param->child->right_sibling->value_id.name,
                CCMMC_SYMBOL_KIND_VARIABLE, func->type.param_list[i]);
        }
    }

    CcmmcAst *child = block->child;
    // Process the list of local declarations
    if (child != NULL && child->type_node == CCMMC_AST_NODE_VARIABLE_DECL_LIST) {
        for (CcmmcAst *var_decl = child->child; var_decl != NULL; var_decl = var_decl->right_sibling)
            any_error = decl_variable(var_decl, table, false) | any_error;
        child = child->right_sibling;
    }
    // Process the list of statements
    if (child != NULL && child->type_node == CCMMC_AST_NODE_STMT_LIST) {
        for (CcmmcAst *stmt = child->child; stmt != NULL; stmt = stmt->right_sibling)
            any_error = process_statement(stmt, table) || any_error;
    }

    // Pop this scope
    ccmmc_symbol_table_close_scope(table);
    return any_error;
}

static bool decl_function(CcmmcAst *func_decl, CcmmcSymbolTable *table)
{
    bool any_error = false;
    CcmmcAst *param_node = func_decl->child->right_sibling->right_sibling;
    size_t param_count = 0;
    CcmmcSymbolType *param_list = NULL;

    // Create an entry for the function in global scope
    if (param_node->child != NULL){
        CcmmcAst *param;
        size_t i;
        for (param = param_node->child;
                param != NULL; param = param->right_sibling, param_count++);
        param_list = malloc(sizeof(CcmmcSymbolType) * param_count);
        ERR_FATAL_CHECK(param_list, malloc);
        for (param = param_node->child, i = 0;
                i < param_count; param = param->right_sibling, i++) {
            param_list[i].type_base = ccmmc_symbol_table_retrieve(table,
                param->child->value_id.name)->type.type_base;
            if (param->child->right_sibling->value_id.kind == CCMMC_KIND_ID_ARRAY) {
                param_list[i].array_size = get_array_size(param->child->right_sibling,
                    &param_list[i].array_dimension);
                if (param_list[i].array_size == NULL)
                    any_error = true;
            }
            else
                param_list[i].array_dimension = 0;
            param_list[i].param_valid = false;
        }
    }
    CcmmcSymbolType func_type = {
        .type_base = ccmmc_symbol_table_retrieve(table,
            func_decl->child->value_id.name)->type.type_base,
        .array_dimension = 0,
        .param_valid = true,
        .param_count = param_count,
        .param_list = param_list };
    ccmmc_symbol_table_insert(table, func_decl->child->right_sibling->value_id.name,
        CCMMC_SYMBOL_KIND_FUNCTION, func_type);

    // process the function block
    return process_block(param_node->right_sibling, table) || any_error;
}

static bool process_program(CcmmcAst *program, CcmmcSymbolTable *table)
{
    bool any_error = false;
    // Process the list of global declarations
    for (CcmmcAst *global_decl = program->child; global_decl != NULL;
         global_decl = global_decl->right_sibling) {
        assert(global_decl->type_node == CCMMC_AST_NODE_DECL);
        switch (global_decl->value_decl.kind) {
            case CCMMC_KIND_DECL_TYPE:
                any_error = decl_typedef(global_decl, table) || any_error;
                break;
            case CCMMC_KIND_DECL_VARIABLE:
                any_error = decl_variable(global_decl, table, true) || any_error;
                break;
            case CCMMC_KIND_DECL_FUNCTION:
                any_error = decl_function(global_decl, table) || any_error;
                break;
            case CCMMC_KIND_DECL_FUNCTION_PARAMETER:
            default:
                assert(false);
        }
    }
    return any_error;
}

bool ccmmc_semantic_check(CcmmcAst *root, CcmmcSymbolTable *table)
{
    bool any_error = false;
    // The symbol table must be empty
    assert(table->all == NULL && table->all_last == NULL);
    assert(table->current == NULL);
    // Push the global scope
    ccmmc_symbol_table_open_scope(table);
    // Insert builtin types
    ccmmc_symbol_table_insert(table, "int", CCMMC_SYMBOL_KIND_TYPE,
        (CcmmcSymbolType){ .type_base = CCMMC_AST_VALUE_INT });
    ccmmc_symbol_table_insert(table, "float", CCMMC_SYMBOL_KIND_TYPE,
        (CcmmcSymbolType){ .type_base = CCMMC_AST_VALUE_FLOAT });
    ccmmc_symbol_table_insert(table, "void", CCMMC_SYMBOL_KIND_TYPE,
        (CcmmcSymbolType){ .type_base = CCMMC_AST_VALUE_VOID });
    // Start processing from the program node
    any_error = process_program(root, table) || any_error;
    return !any_error;
}

// vim: set sw=4 ts=4 sts=4 et:
