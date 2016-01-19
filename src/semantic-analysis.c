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
#define WARNING(msg) ("Warning in line %zu\n" msg "\n")


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

static bool check_relop_expr(CcmmcAst *expr, CcmmcSymbolTable *table);

static bool check_array_subscript(CcmmcAst *ref, CcmmcSymbolTable *table,
    size_t *array_dimension)
{
    bool any_error = false;
    size_t count = 0;
    for (CcmmcAst *dim = ref->child; dim != NULL; dim = dim->right_sibling, count++) {
        any_error = check_relop_expr(dim, table) || any_error;
        if (dim->type_value == CCMMC_AST_VALUE_ERROR)
            continue;
        if (dim->type_value == CCMMC_AST_VALUE_FLOAT ||
            dim->type_value == CCMMC_AST_VALUE_VOID) {
            any_error = true;
            fprintf(stderr, ERROR("Array subscript is not an integer."),
                dim->line_number);
            continue;
        }
        assert(dim->type_value == CCMMC_AST_VALUE_INT);
    }
    if (array_dimension != NULL)
        *array_dimension = count;
    return any_error;
}

static bool check_var_ref(CcmmcAst*, CcmmcSymbolTable*);

static bool check_call(CcmmcAst *call, CcmmcSymbolTable *table)
{
    bool any_error = false;

    // Check function symbol
    CcmmcSymbol *func = ccmmc_symbol_table_retrieve(table,
        call->child->value_id.name);
    if (func == NULL) {
        fprintf(stderr, ERROR("ID `%s' undeclared."),
            call->child->line_number, call->child->value_id.name);
        return true;
    }
    if (func->kind != CCMMC_SYMBOL_KIND_FUNCTION) {
        fprintf(stderr, ERROR("ID `%s' is not a function."),
            call->child->line_number, func->name);
        return true;
    }

    // Check param_count
    CcmmcAst *param = call->child->right_sibling->child;
    size_t param_count = 0;
    for (; param != NULL; param = param->right_sibling, param_count++);
    if (param_count < func->type.param_count) {
        fprintf(stderr, ERROR("Too few arguments to function `%s'."),
            call->child->line_number, func->name);
        return true;
    }
    if (param_count > func->type.param_count) {
        fprintf(stderr, ERROR("Too many arguments to function `%s'."),
            call->child->line_number, func->name);
        return true;
    }

    // Check special function: write
    if (strcmp(func->name, "write") == 0) {
        CcmmcAst *arg = call->child->right_sibling->child;
        call->type_value = func->type.type_base;
        if (arg->type_node == CCMMC_AST_NODE_CONST_VALUE &&
            arg->value_const.kind == CCMMC_KIND_CONST_STRING) {
            arg->type_value = CCMMC_AST_VALUE_CONST_STRING;
            return any_error;
        }
        any_error = check_relop_expr(arg, table) || any_error;
        return any_error;
    }

    // Check each parameter
    param = call->child->right_sibling->child;
    size_t i = 0;
    for (; i < param_count; param = param->right_sibling, i++) {
        if (param->type_node == CCMMC_AST_NODE_ID) {
            CcmmcSymbol *param_sym = ccmmc_symbol_table_retrieve(table,
                param->value_id.name);
            if (param_sym == NULL) {
                fprintf(stderr, ERROR("ID `%s' undeclared."),
                    param->line_number, param->value_id.name);
                any_error = true;
                continue;
            }
            if (param_sym->kind != CCMMC_SYMBOL_KIND_VARIABLE) {
                fprintf(stderr, ERROR("ID `%s' is not a variable."),
                    param->line_number, param_sym->name);
                any_error = true;
                continue;
            }
            size_t dim;
            any_error = check_array_subscript(param, table, &dim) || any_error;
            if (dim > param_sym->type.array_dimension) {
                fprintf(stderr, ERROR("Incompatible array dimensions."), param->line_number);
                any_error = true;
                continue;
            }
            if (func->type.param_list[i].array_dimension == 0 &&
                    dim < param_sym->type.array_dimension) {
                fprintf(stderr, ERROR("Array `%s' passed to scalar parameter %zu."),
                    param->line_number, param_sym->name, i);
                any_error = true;
                continue;
            }
            if (func->type.param_list[i].array_dimension != 0 &&
                    dim == param_sym->type.array_dimension) {
                fprintf(stderr, ERROR("Scalar `%s' passed to array parameter %zu."),
                    param->line_number, param_sym->name, i);
                any_error = true;
                continue;
            }
            // Get the type of the scalar variable for later use
            if (func->type.param_list[i].array_dimension == 0)
                any_error = check_relop_expr(param, table) || any_error;
        }
        else {
            any_error = check_relop_expr(param, table) || any_error;
        }
    }

    // Fill return type in the node of function call
    call->type_value = func->type.type_base;
    return any_error;
}

static bool check_array_ref(CcmmcAst *ref, CcmmcSymbolTable *table, CcmmcSymbol *symbol)
{
    bool any_error = false;
    if (symbol->type.array_size == NULL)
        return false;

    CcmmcAst *dim;
    size_t dim_count = 0;
    for (dim = ref->child; dim != NULL; dim = dim->right_sibling, dim_count++);
    assert(dim_count != 0);

    if (symbol->type.array_dimension != dim_count) {
        fprintf(stderr, ERROR("Incompatible array dimensions."), ref->line_number);
        return true;
    }
    any_error = check_array_subscript(ref, table, NULL) || any_error;
    return any_error;
}

static bool check_var_ref(CcmmcAst *ref, CcmmcSymbolTable *table)
{
    CcmmcSymbol *symbol = ccmmc_symbol_table_retrieve(table, ref->value_id.name);
    if (symbol == NULL) {
        fprintf(stderr, ERROR("ID `%s' undeclared."),
            ref->line_number, ref->value_id.name);
        ref->type_value = CCMMC_AST_VALUE_ERROR;
        return true;
    }
    if (symbol->kind != CCMMC_SYMBOL_KIND_VARIABLE) {
        fprintf(stderr, ERROR("ID `%s' is not a variable."),
            ref->line_number, ref->value_id.name);
        ref->type_value = CCMMC_AST_VALUE_ERROR;
        return true;
    }
    assert(symbol->type.type_base == CCMMC_AST_VALUE_INT ||
           symbol->type.type_base == CCMMC_AST_VALUE_FLOAT);

    switch (ref->value_id.kind) {
        case CCMMC_KIND_ID_NORMAL:
            ref->type_value = symbol->type.type_base;
            if (!ccmmc_symbol_is_scalar(symbol)) {
                fprintf(stderr, ERROR("ID `%s' is not a scalar"),
                    ref->line_number, ref->value_id.name);
                return true;
            }
            return false;
        case CCMMC_KIND_ID_ARRAY:
            ref->type_value = symbol->type.type_base;
            if (!ccmmc_symbol_is_array(symbol)) {
                fprintf(stderr, ERROR("ID `%s' is not an array"),
                    ref->line_number, ref->value_id.name);
                return true;
            }
            return check_array_ref(ref, table, symbol);
        case CCMMC_KIND_ID_WITH_INIT:
        default:
            assert(false);
    }
}

static bool check_relop_expr(CcmmcAst *expr, CcmmcSymbolTable *table)
{
    if (expr->type_node == CCMMC_AST_NODE_CONST_VALUE) {
        switch (expr->value_const.kind) {
            case CCMMC_KIND_CONST_INT:
                expr->type_value = CCMMC_AST_VALUE_INT;
                break;
            case CCMMC_KIND_CONST_FLOAT:
                expr->type_value = CCMMC_AST_VALUE_FLOAT;
                break;
            case CCMMC_KIND_CONST_STRING:
                fprintf(stderr, ERROR("Strings are not allowed in expressions."),
                    expr->line_number);
                expr->type_value = CCMMC_AST_VALUE_ERROR;
                return true;
            case CCMMC_KIND_CONST_ERROR:
                expr->type_value = CCMMC_AST_VALUE_ERROR;
                return true;
            default:
                assert(false);
        }
        return false;
    }

    if (expr->type_node == CCMMC_AST_NODE_STMT &&
        expr->value_stmt.kind == CCMMC_KIND_STMT_FUNCTION_CALL) {
        bool any_error = check_call(expr, table);
        if (expr->type_value == CCMMC_AST_VALUE_VOID) {
            fprintf(stderr, ERROR(
                "Cannot use void type ID `%s' in expressions."),
                expr->line_number, expr->child->value_id.name);
            any_error = true;
        }
        return any_error;
    }

    if (expr->type_node == CCMMC_AST_NODE_ID)
        return check_var_ref(expr, table);

    assert(expr->type_node == CCMMC_AST_NODE_EXPR);

    if (expr->value_expr.kind == CCMMC_KIND_EXPR_BINARY_OP) {
        CcmmcAst *left = expr->child;
        CcmmcAst *right = expr->child->right_sibling;
        if (check_relop_expr(left, table) ||
            check_relop_expr(right, table)) {
            expr->type_value = CCMMC_AST_VALUE_ERROR;
            return true;
        }
        if (left->type_value != CCMMC_AST_VALUE_INT &&
            left->type_value != CCMMC_AST_VALUE_FLOAT)
            assert(false);
        if (right->type_value != CCMMC_AST_VALUE_INT &&
            right->type_value != CCMMC_AST_VALUE_FLOAT)
            assert(false);

        switch (expr->value_expr.op_binary) {
            case CCMMC_KIND_OP_BINARY_ADD:
            case CCMMC_KIND_OP_BINARY_SUB:
            case CCMMC_KIND_OP_BINARY_MUL:
            case CCMMC_KIND_OP_BINARY_DIV:
                if (left->type_value == CCMMC_AST_VALUE_INT &&
                    right->type_value == CCMMC_AST_VALUE_INT)
                    expr->type_value = CCMMC_AST_VALUE_INT;
                else
                    expr->type_value = CCMMC_AST_VALUE_FLOAT;
                return false;
            case CCMMC_KIND_OP_BINARY_EQ:
            case CCMMC_KIND_OP_BINARY_GE:
            case CCMMC_KIND_OP_BINARY_LE:
            case CCMMC_KIND_OP_BINARY_NE:
            case CCMMC_KIND_OP_BINARY_GT:
            case CCMMC_KIND_OP_BINARY_LT:
            case CCMMC_KIND_OP_BINARY_AND:
            case CCMMC_KIND_OP_BINARY_OR:
                expr->type_value = CCMMC_AST_VALUE_INT;
                return false;
            default:
                assert(false);
        }
    }

    if (expr->value_expr.kind == CCMMC_KIND_EXPR_UNARY_OP) {
        CcmmcAst *arg = expr->child;
        if (check_relop_expr(arg, table)) {
            expr->type_value = CCMMC_AST_VALUE_ERROR;
            return true;
        }
        if (arg->type_value != CCMMC_AST_VALUE_INT &&
            arg->type_value != CCMMC_AST_VALUE_FLOAT)
            assert(false);

        switch (expr->value_expr.op_unary) {
            case CCMMC_KIND_OP_UNARY_POSITIVE:
            case CCMMC_KIND_OP_UNARY_NEGATIVE:
                expr->type_value = arg->type_value;
                return false;
            case CCMMC_KIND_OP_UNARY_LOGICAL_NEGATION:
                expr->type_value = CCMMC_AST_VALUE_INT;
                return false;
            default:
                assert(false);
        }
    }

    assert(false);
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
        init_id->type_value = type_sym->type.type_base;
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
    CcmmcSymbol *func_sym;

    if (stmt->type_node == CCMMC_AST_NODE_NUL)
        return false;
    if (stmt->type_node == CCMMC_AST_NODE_BLOCK)
        return process_block(stmt, table) || any_error;
    assert(stmt->type_node == CCMMC_AST_NODE_STMT);

    switch(stmt->value_stmt.kind) {
        case CCMMC_KIND_STMT_WHILE:
            any_error = check_relop_expr(stmt->child, table) || any_error;
            any_error = process_statement(stmt->child->right_sibling,
                table) || any_error;
            break;
        case CCMMC_KIND_STMT_FOR:
            for (CcmmcAst *assign = stmt->child->child; assign != NULL;
                    assign = assign->right_sibling) {
                any_error = process_statement(assign, table) || any_error;
            }
            for (CcmmcAst *expr = stmt->child->right_sibling->child; expr != NULL;
                    expr = expr->right_sibling) {
                any_error = check_relop_expr(expr, table) || any_error;
            }
            for (CcmmcAst *assign = stmt->child->right_sibling->right_sibling->child;
                    assign != NULL; assign = assign->right_sibling) {
                any_error = process_statement(assign, table) || any_error;
            }
            any_error = process_statement(
                stmt->child->right_sibling->right_sibling->right_sibling,
                table) || any_error;
            break;
        case CCMMC_KIND_STMT_ASSIGN:
            any_error = check_var_ref(stmt->child, table) || any_error;
            any_error = check_relop_expr(stmt->child->right_sibling,
                table) || any_error;
            break;
        case CCMMC_KIND_STMT_IF:
            any_error = check_relop_expr(stmt->child, table) || any_error;
            any_error = process_statement(stmt->child->right_sibling, table)
                || any_error;
            if (stmt->child->right_sibling->right_sibling->type_node
                != CCMMC_AST_NODE_NUL)
                    any_error = process_statement(
                        stmt->child->right_sibling->right_sibling,
                        table) || any_error;
            break;
        case CCMMC_KIND_STMT_FUNCTION_CALL:
            any_error = check_call(stmt, table) || any_error;
            break;
        case CCMMC_KIND_STMT_RETURN:
            for (CcmmcAst *func = stmt->parent; ; func = func->parent) {
                if (func->type_node == CCMMC_AST_NODE_DECL &&
                    func->value_decl.kind == CCMMC_KIND_DECL_FUNCTION) {
                    func_sym = ccmmc_symbol_table_retrieve(table,
                        func->child->right_sibling->value_id.name);
                    break;
                }
            }
            if (stmt->child == NULL) {
                if (func_sym->type.type_base != CCMMC_AST_VALUE_VOID)
                    fprintf(stderr, WARNING("Incompatible return type."),
                        stmt->line_number);
            }
            else {
                any_error = check_relop_expr(stmt->child, table) || any_error;
                if (func_sym->type.type_base != stmt->child->type_value) {
                    fprintf(stderr, WARNING("Incompatible return type."),
                        stmt->line_number);
                }
            }
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
            any_error = decl_variable(var_decl, table, false) || any_error;
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

    // Check if redeclared
    if (ccmmc_symbol_scope_exist(table->current,
            func_decl->child->right_sibling->value_id.name)) {
        fprintf (stderr, ERROR("ID `%s' redeclared."),
            func_decl->line_number,
            func_decl->child->right_sibling->value_id.name);
        return true;
    }

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
    // Insert builtin functions
    ccmmc_symbol_table_insert(table, "read", CCMMC_SYMBOL_KIND_FUNCTION,
        (CcmmcSymbolType){ .type_base = CCMMC_AST_VALUE_INT,
            .param_valid = true, .param_count = 0 });
    ccmmc_symbol_table_insert(table, "fread", CCMMC_SYMBOL_KIND_FUNCTION,
        (CcmmcSymbolType){ .type_base = CCMMC_AST_VALUE_FLOAT,
            .param_valid = true, .param_count = 0 });
    ccmmc_symbol_table_insert(table, "write", CCMMC_SYMBOL_KIND_FUNCTION,
        (CcmmcSymbolType){ .type_base = CCMMC_AST_VALUE_VOID,
            .param_valid = true, .param_count = 1,
            .param_list = (CcmmcSymbolType[]){
                { .type_base = CCMMC_AST_VALUE_INT }}});
    // Start processing from the program node
    any_error = process_program(root, table) || any_error;
    return !any_error;
}

// vim: set sw=4 ts=4 sts=4 et:
