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
    if (expr->type_node == CCMMC_AST_NODE_CONST_VALUE)
        return expr->value_const;
    assert(expr->type_node == CCMMC_AST_NODE_EXPR);
    assert(expr->value_expr.kind == CCMMC_KIND_EXPR_BINARY_OP);
    CcmmcValueConst left = eval_const_expr(expr->child);
    CcmmcValueConst right = eval_const_expr(expr->child->right_sibling);
    if (left.kind == CCMMC_KIND_CONST_ERROR ||
        right.kind == CCMMC_KIND_CONST_ERROR)
        return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_ERROR };
    switch (expr->value_expr.op_binary) {
        case CCMMC_KIND_OP_BINARY_ADD:
            if (left.kind == CCMMC_KIND_CONST_INT) {
                if (right.kind == CCMMC_KIND_CONST_INT) {
                    return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_INT,
                        .const_int = left.const_int + right.const_int };
                } else {
                    float left_value = left.const_int;
                    float right_value = right.const_float;
                    return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_FLOAT,
                        .const_float = left_value + right_value };
                }
            } else {
                if (right.kind == CCMMC_KIND_CONST_INT) {
                    float left_value = left.const_float;
                    float right_value = right.const_int;
                    return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_FLOAT,
                        .const_float = left_value + right_value };
                } else {
                    return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_FLOAT,
                        .const_float = left.const_float + right.const_float };
                }
            }
        case CCMMC_KIND_OP_BINARY_SUB:
            if (left.kind == CCMMC_KIND_CONST_INT) {
                if (right.kind == CCMMC_KIND_CONST_INT) {
                    return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_INT,
                        .const_int = left.const_int - right.const_int };
                } else {
                    float left_value = left.const_int;
                    float right_value = right.const_float;
                    return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_FLOAT,
                        .const_float = left_value - right_value };
                }
            } else {
                if (right.kind == CCMMC_KIND_CONST_INT) {
                    float left_value = left.const_float;
                    float right_value = right.const_int;
                    return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_FLOAT,
                        .const_float = left_value - right_value };
                } else {
                    return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_FLOAT,
                        .const_float = left.const_float - right.const_float };
                }
            }
        case CCMMC_KIND_OP_BINARY_MUL:
            if (left.kind == CCMMC_KIND_CONST_INT) {
                if (right.kind == CCMMC_KIND_CONST_INT) {
                    return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_INT,
                        .const_int = left.const_int * right.const_int };
                } else {
                    float left_value = left.const_int;
                    float right_value = right.const_float;
                    return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_FLOAT,
                        .const_float = left_value * right_value };
                }
            } else {
                if (right.kind == CCMMC_KIND_CONST_INT) {
                    float left_value = left.const_float;
                    float right_value = right.const_int;
                    return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_FLOAT,
                        .const_float = left_value * right_value };
                } else {
                    return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_FLOAT,
                        .const_float = left.const_float * right.const_float };
                }
            }
        case CCMMC_KIND_OP_BINARY_DIV:
            if (left.kind == CCMMC_KIND_CONST_INT) {
                if (right.kind == CCMMC_KIND_CONST_INT) {
                    if (right.const_int == 0) {
                        fprintf(stderr, ERROR("Integer division by zero."),
                            expr->line_number);
                        return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_ERROR };
                    }
                    return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_INT,
                        .const_int = left.const_int / right.const_int };
                } else {
                    float left_value = left.const_int;
                    float right_value = right.const_float;
                    return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_FLOAT,
                        .const_float = left_value / right_value };
                }
            } else {
                if (right.kind == CCMMC_KIND_CONST_INT) {
                    float left_value = left.const_float;
                    float right_value = right.const_int;
                    return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_FLOAT,
                        .const_float = left_value / right_value };
                } else {
                    return (CcmmcValueConst){ .kind = CCMMC_KIND_CONST_FLOAT,
                        .const_float = left.const_float / right.const_float };
                }
            }
        case CCMMC_KIND_OP_BINARY_EQ:
        case CCMMC_KIND_OP_BINARY_GE:
        case CCMMC_KIND_OP_BINARY_LE:
        case CCMMC_KIND_OP_BINARY_NE:
        case CCMMC_KIND_OP_BINARY_GT:
        case CCMMC_KIND_OP_BINARY_LT:
        case CCMMC_KIND_OP_BINARY_AND:
        case CCMMC_KIND_OP_BINARY_OR:
        default:
            assert(false);
    }
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

static bool process_typedef(CcmmcAst *type_decl, CcmmcSymbolTable *table)
{
    bool any_error = false;

    // The leftmost child is an existing type
    assert(type_decl->child != NULL);
    assert(type_decl->child->type_node == CCMMC_AST_NODE_ID);
    assert(type_decl->child->value_id.kind == CCMMC_KIND_ID_NORMAL);
    const char *source_str = type_decl->child->value_id.name;
    CcmmcSymbol *source_sym = ccmmc_symbol_table_retrive(table, source_str);
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

static bool process_relop_expr(CcmmcAst *expr, CcmmcSymbolTable *table)
{
    bool any_error = false;
    return any_error;
}

static bool process_variable(CcmmcAst *var_decl, CcmmcSymbolTable *table)
{
    bool any_error = false;

    // The leftmost child is the type of the variable declaration list
    assert(var_decl->child != NULL);
    assert(var_decl->child->type_node == CCMMC_AST_NODE_ID);
    assert(var_decl->child->value_id.kind == CCMMC_KIND_ID_NORMAL);
    const char *type_str = var_decl->child->value_id.name;
    CcmmcSymbol *type_sym = ccmmc_symbol_table_retrive(table, type_str);
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
                if (process_relop_expr(init_id, table)) {
                    any_error = true;
                    continue;
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

static bool process_function(CcmmcAst *function_decl, CcmmcSymbolTable *table)
{
    bool any_error = false;
    return any_error;
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
                any_error = process_typedef(global_decl, table) || any_error;
                break;
            case CCMMC_KIND_DECL_VARIABLE:
                any_error = process_variable(global_decl, table) || any_error;
                break;
            case CCMMC_KIND_DECL_FUNCTION:
                any_error = process_function(global_decl, table) || any_error;
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
