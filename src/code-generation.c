#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "code-generation.h"
#include "register.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static void generate_global_variable(CcmmcAst *global_decl, CcmmcState *state)
{
    fputs("\t.data\n\t.align\t2\n", state->asm_output);
    for (CcmmcAst *var_decl = global_decl->child->right_sibling;
         var_decl != NULL; var_decl = var_decl->right_sibling) {
        CcmmcSymbol *var_sym = ccmmc_symbol_table_retrieve(state->table,
            var_decl->value_id.name);
        switch (var_decl->value_id.kind) {
            case CCMMC_KIND_ID_NORMAL:
                fprintf(state->asm_output, "\t.comm\t%s, 4\n", var_sym->name);
                break;
            case CCMMC_KIND_ID_ARRAY: {
                size_t total_elements = 1;
                assert(var_sym->type.array_dimension > 0);
                for (size_t i = 0; i < var_sym->type.array_dimension; i++)
                    total_elements *= var_sym->type.array_size[i];
                fprintf(state->asm_output, "\t.comm\t%s, %zu\n",
                    var_sym->name, total_elements * 4);
                } break;
            case CCMMC_KIND_ID_WITH_INIT: {
                CcmmcAst *init_value = var_decl->child;
                fprintf(state->asm_output,
                    "\t.type\t%s, %%object\n"
                    "\t.size\t%s, 4\n"
                    "\t.global\t%s\n",
                    var_sym->name, var_sym->name, var_sym->name);
                if (var_sym->type.type_base == CCMMC_AST_VALUE_INT) {
                    int int_value;
                    if (init_value->type_node == CCMMC_AST_NODE_CONST_VALUE) {
                        assert(init_value->value_const.kind == CCMMC_KIND_CONST_INT);
                        int_value = init_value->value_const.const_int;
                    } else if (init_value->type_node == CCMMC_AST_NODE_EXPR) {
                        assert(ccmmc_ast_expr_get_is_constant(init_value));
                        assert(ccmmc_ast_expr_get_is_int(init_value));
                        int_value = ccmmc_ast_expr_get_int(init_value);
                    } else {
                        assert(false);
                    }
                    fprintf(state->asm_output,
                        "%s:\n"
                        "\t.word\t%d\n",
                        var_sym->name, int_value);
                } else if (var_sym->type.type_base == CCMMC_AST_VALUE_FLOAT) {
                    float float_value;
                    if (init_value->type_node == CCMMC_AST_NODE_CONST_VALUE) {
                        assert(init_value->value_const.kind == CCMMC_KIND_CONST_FLOAT);
                        float_value = init_value->value_const.const_float;
                    } else if (init_value->type_node == CCMMC_AST_NODE_EXPR) {
                        assert(ccmmc_ast_expr_get_is_constant(init_value));
                        assert(ccmmc_ast_expr_get_is_float(init_value));
                        float_value = ccmmc_ast_expr_get_float(init_value);
                    } else {
                        assert(false);
                    }
                    fprintf(state->asm_output,
                        "%s:\n"
                        "\t.float\t%.9g\n",
                       var_sym->name, float_value);
                } else {
                    assert(false);
                }
                } break;
            default:
                assert(false);
        }
    }
}

static inline bool safe_immediate(uint64_t imm) {
    return imm <= 4096;
}

static inline void load_variable(CcmmcAst *id, CcmmcState *state, const char *r) {
    fprintf(state->asm_output, "\t/* var load, line %zu */\n", id->line_number);
    const char *var_name = id->value_id.name;
    CcmmcSymbol *var_sym = ccmmc_symbol_table_retrieve(state->table, var_name);
    // TODO: array
    if (ccmmc_symbol_attr_is_global(&var_sym->attr)) {
        fprintf(state->asm_output,
            "\tldr\t%s, %s\n", r, var_name);
    } else {
        if (safe_immediate(var_sym->attr.addr)) {
            fprintf(state->asm_output,
                "\tldr\t%s, [fp, #-%" PRIu64 "]\n", r, var_sym->attr.addr);
        } else {
            // TODO: large offset
            assert(false);
        }
    }
}

static inline void store_variable(CcmmcAst *id, CcmmcState *state, const char *r) {
    fprintf(state->asm_output, "\t/* var store, line %zu */\n", id->line_number);
    const char *var_name = id->value_id.name;
    CcmmcSymbol *var_sym = ccmmc_symbol_table_retrieve(state->table, var_name);
    // TODO: array
    if (ccmmc_symbol_attr_is_global(&var_sym->attr)) {
        fprintf(state->asm_output,
            "\tstr\t%s, %s\n", r, var_name);
    } else {
        if (safe_immediate(var_sym->attr.addr)) {
            fprintf(state->asm_output,
                "\tstr\t%s, [fp, #-%" PRIu64 "]\n", r, var_sym->attr.addr);
        } else {
            // TODO: large offset
            assert(false);
        }
    }
}

static void generate_expression(CcmmcAst *expr, CcmmcState *state,
    const char *result, const char *op1, const char *op2)
{
    if (expr->type_node == CCMMC_AST_NODE_CONST_VALUE) {
        fprintf(state->asm_output,
            "\t/* const value, line %zu */\n", expr->line_number);
        if (expr->type_value == CCMMC_AST_VALUE_INT) {
            fprintf(state->asm_output,
                "\tldr\t%s, =%d\n", result, expr->value_const.const_int);
        } else if (expr->type_value == CCMMC_AST_VALUE_FLOAT) {
            fprintf(state->asm_output,
                "\tldr\t%s, .LC%zu\n"
                "\t.section .rodata\n"
                ".LC%zu:\n"
                "\t.float\t%.9g\n"
                "\t.text\n",
                result, state->label_number,
                state->label_number,
                expr->value_const.const_float);
            state->label_number++;
        } else {
            assert(false);
        }
        return;
    }

    if (expr->type_node == CCMMC_AST_NODE_STMT &&
        expr->value_stmt.kind == CCMMC_KIND_STMT_FUNCTION_CALL) {
        // TODO: function call
        assert(false);
    }

    if (expr->type_node == CCMMC_AST_NODE_ID) {
        load_variable(expr, state, result);
        return;
    }

    assert(expr->type_node == CCMMC_AST_NODE_EXPR);

#define FPREG_RESULT  "s16"
#define FPREG_OP1     "s17"
#define FPREG_OP2     "s18"

    if (expr->value_expr.kind == CCMMC_KIND_EXPR_BINARY_OP) {
        CcmmcAst *left = expr->child;
        CcmmcAst *right = expr->child->right_sibling;
        size_t label_short = state->label_number;

        // evaluate the left expression
        fprintf(state->asm_output,
            "\t/* expr binary op, line %zu */\n", expr->line_number);
        generate_expression(left, state, op1, op2, result);
        if (expr->value_expr.op_binary == CCMMC_KIND_OP_BINARY_AND) {
            // logical AND needs short-curcuit evaluation
            label_short = state->label_number++;
            if (left->type_value == CCMMC_AST_VALUE_FLOAT)
                fprintf(state->asm_output,
                    "\tfmov\t%s, %s\n"
                    "\tfcmp\t%s, #0.0\n"
                    "\tb.eq\t.LC%zu\n",
                    FPREG_OP1, op1,
                    FPREG_OP1,
                    label_short);
            else
                fprintf(state->asm_output,
                    "\tcbz\t%s, .LC%zu\n",
                    op1, label_short);
        } else if (expr->value_expr.op_binary == CCMMC_KIND_OP_BINARY_OR) {
            // logical OR needs short-curcuit evaluation
            label_short = state->label_number++;
            if (left->type_value == CCMMC_AST_VALUE_FLOAT)
                fprintf(state->asm_output,
                    "\tfmov\t%s, %s\n"
                    "\tfcmp\t%s, #0.0\n"
                    "\tb.ne\t.LC%zu\n",
                    FPREG_OP1, op1,
                    FPREG_OP1,
                    label_short);
            else
                fprintf(state->asm_output,
                    "\tcbnz\t%s, .LC%zu\n",
                    op1, label_short);
        }

        // evaluate the right expression
        fprintf(state->asm_output,
            "\tsub\tsp, sp, #4 /* save left op */\n"
            "\tstr\t%s, [sp] /* save left op */\n",
            op1);
        generate_expression(right, state, op2, result, op1);
        fprintf(state->asm_output,
            "\tldr\t%s, [sp] /* restore left op */\n"
            "\tadd\tsp, sp, #4 /* restore left op */\n",
            op1);

        if (left->type_value == CCMMC_AST_VALUE_FLOAT ||
            right->type_value == CCMMC_AST_VALUE_FLOAT) {
            if (left->type_value == CCMMC_AST_VALUE_INT)
                fprintf(state->asm_output, "\tscvtf\t%s, %s\n", FPREG_OP1, op1);
            else if (left->type_value == CCMMC_AST_VALUE_FLOAT)
                fprintf(state->asm_output, "\tfmov\t%s, %s\n", FPREG_OP1, op1);
            else
                assert(false);
            if (right->type_value == CCMMC_AST_VALUE_INT)
                fprintf(state->asm_output, "\tscvtf\t%s, %s\n", FPREG_OP2, op2);
            else if (right->type_value == CCMMC_AST_VALUE_FLOAT)
                fprintf(state->asm_output, "\tfmov\t%s, %s\n", FPREG_OP2, op2);
            else
                assert(false);
        }

        switch (expr->value_expr.op_binary) {
            case CCMMC_KIND_OP_BINARY_ADD:
                if (expr->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfadd\t%s, %s, %s\n",
                        FPREG_RESULT, FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tadd\t%s, %s, %s\n",
                        result, op1, op2);
                break;
            case CCMMC_KIND_OP_BINARY_SUB:
                if (expr->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfsub\t%s, %s, %s\n",
                        FPREG_RESULT, FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tsub\t%s, %s, %s\n",
                        result, op1, op2);
                break;
            case CCMMC_KIND_OP_BINARY_MUL:
                if (expr->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfmul\t%s, %s, %s\n",
                        FPREG_RESULT, FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tmul\t%s, %s, %s\n",
                        result, op1, op2);
                break;
            case CCMMC_KIND_OP_BINARY_DIV:
                if (expr->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfdiv\t%s, %s, %s\n",
                        FPREG_RESULT, FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tdiv\t%s, %s, %s\n",
                        result, op1, op2);
                break;
            case CCMMC_KIND_OP_BINARY_EQ:
                if (left->type_value == CCMMC_AST_VALUE_FLOAT ||
                    right->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfcmp\t%s, %s\n", FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tcmp\t%s, %s\n", op1, op2);
                fprintf(state->asm_output, "\tcset\t%s, eq\n", result);
                break;
            case CCMMC_KIND_OP_BINARY_GE:
                if (left->type_value == CCMMC_AST_VALUE_FLOAT ||
                    right->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfcmp\t%s, %s\n", FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tcmp\t%s, %s\n", op1, op2);
                fprintf(state->asm_output, "\tcset\t%s, ge\n", result);
                break;
            case CCMMC_KIND_OP_BINARY_LE:
                if (left->type_value == CCMMC_AST_VALUE_FLOAT ||
                    right->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfcmp\t%s, %s\n", FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tcmp\t%s, %s\n", op1, op2);
                fprintf(state->asm_output, "\tcset\t%s, le\n", result);
                break;
            case CCMMC_KIND_OP_BINARY_NE:
                if (left->type_value == CCMMC_AST_VALUE_FLOAT ||
                    right->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfcmp\t%s, %s\n", FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tcmp\t%s, %s\n", op1, op2);
                fprintf(state->asm_output, "\tcset\t%s, ne\n", result);
                break;
            case CCMMC_KIND_OP_BINARY_GT:
                if (left->type_value == CCMMC_AST_VALUE_FLOAT ||
                    right->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfcmp\t%s, %s\n", FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tcmp\t%s, %s\n", op1, op2);
                fprintf(state->asm_output, "\tcset\t%s, gt\n", result);
                break;
            case CCMMC_KIND_OP_BINARY_LT:
                if (left->type_value == CCMMC_AST_VALUE_FLOAT ||
                    right->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfcmp\t%s, %s\n", FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tcmp\t%s, %s\n", op1, op2);
                fprintf(state->asm_output, "\tcset\t%s, lt\n", result);
                break;
            case CCMMC_KIND_OP_BINARY_AND: {
                size_t label_exit = state->label_number++;
                if (right->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output,
                        "\tfcmp\t%s, #0.0\n"
                        "\tb.eq\t.LC%zu\n"
                        "\tmov\t%s, #1\n"
                        "\tb\t.LC%zu\n"
                        ".LC%zu:\n"
                        "\tmov\t%s, #0\n"
                        ".LC%zu:\n",
                        FPREG_OP2,
                        label_short,
                        result,
                        label_exit,
                        label_short,
                        result,
                        label_exit);
                else
                    fprintf(state->asm_output,
                        "\tcbz\t%s, .LC%zu\n"
                        "\tmov\t%s, #1\n"
                        "\tb\t.LC%zu\n"
                        ".LC%zu:\n"
                        "\tmov\t%s, #0\n"
                        ".LC%zu:\n",
                        op2, label_short,
                        result,
                        label_exit,
                        label_short,
                        result,
                        label_exit);
                } break;
            case CCMMC_KIND_OP_BINARY_OR: {
                size_t label_exit = state->label_number++;
                if (right->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output,
                        "\tfcmp\t%s, #0.0\n"
                        "\tb.ne\t.LC%zu\n"
                        "\tmov\t%s, #0\n"
                        "\tb\t.LC%zu\n"
                        ".LC%zu:\n"
                        "\tmov\t%s, #1\n"
                        ".LC%zu:\n",
                        FPREG_OP2,
                        label_short,
                        result,
                        label_exit,
                        label_short,
                        result,
                        label_exit);
                else
                    fprintf(state->asm_output,
                        "\tcbnz\t%s, .LC%zu\n"
                        "\tmov\t%s, #0\n"
                        "\tb\t.LC%zu\n"
                        ".LC%zu:\n"
                        "\tmov\t%s, #1\n"
                        ".LC%zu:\n",
                        op2, label_short,
                        result,
                        label_exit,
                        label_short,
                        result,
                        label_exit);
                } break;
            default:
                assert(false);
        }

        if (expr->type_value == CCMMC_AST_VALUE_FLOAT)
            fprintf(state->asm_output, "\tfmov\t%s, %s\n", result, FPREG_RESULT);
        return;
    }

    if (expr->value_expr.kind == CCMMC_KIND_EXPR_UNARY_OP) {
        CcmmcAst *arg = expr->child;

        fprintf(state->asm_output,
            "\t/* expr unary op, line %zu */\n", expr->line_number);
        generate_expression(arg, state, op1, op2, result);

        if (arg->type_value == CCMMC_AST_VALUE_FLOAT)
            fprintf(state->asm_output, "\tfmov\t%s, %s\n", FPREG_OP1, op1);

        switch (expr->value_expr.op_unary) {
            case CCMMC_KIND_OP_UNARY_POSITIVE:
                fputs("\t/* nop */\n", state->asm_output);
                break;
            case CCMMC_KIND_OP_UNARY_NEGATIVE:
                if (arg->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfneg\t%s, %s\n", FPREG_RESULT, op1);
                else
                    fprintf(state->asm_output, "\tneg\t%s, %s\n", result, op1);
                break;
            case CCMMC_KIND_OP_UNARY_LOGICAL_NEGATION:
                if (arg->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfcmp\t%s, #0.0\n", FPREG_OP1);
                else
                    fprintf(state->asm_output, "\tcmp\t%s, wzr\n", op1);
                fprintf(state->asm_output, "\tcset\t%s, eq\n", result);
                break;
            default:
                assert(false);
        }

        if (expr->type_value == CCMMC_AST_VALUE_FLOAT)
            fprintf(state->asm_output, "\tfmov\t%s, %s\n", result, FPREG_RESULT);
        return;
    }

#undef FPREG_RESULT
#undef FPREG_OP1
#undef FPREG_OP2

    assert(false);
}

static void generate_block(
    CcmmcAst *block, CcmmcState *state, uint64_t current_offset);
static void generate_statement(
    CcmmcAst *stmt, CcmmcState *state, uint64_t current_offset)
{
    if (stmt->type_node == CCMMC_AST_NODE_NUL)
        return;
    if (stmt->type_node == CCMMC_AST_NODE_BLOCK) {
        generate_block(stmt, state, current_offset);
        return;
    }

    assert(stmt->type_node == CCMMC_AST_NODE_STMT);
    switch(stmt->value_stmt.kind) {
        case CCMMC_KIND_STMT_WHILE:
            generate_statement(stmt->child->right_sibling,
                state, current_offset);
            break;
        case CCMMC_KIND_STMT_FOR:
            break;
        case CCMMC_KIND_STMT_ASSIGN: {
#define FPREG_TMP  "s16"
            CcmmcTmp *tmp1 = ccmmc_register_alloc(state->reg_pool, &current_offset);
            CcmmcTmp *tmp2 = ccmmc_register_alloc(state->reg_pool, &current_offset);
            CcmmcTmp *tmp3 = ccmmc_register_alloc(state->reg_pool, &current_offset);
            const char *result = ccmmc_register_lock(state->reg_pool, tmp1);
            const char *op1 = ccmmc_register_lock(state->reg_pool, tmp2);
            const char *op2 = ccmmc_register_lock(state->reg_pool, tmp3);
            CcmmcAst *lvar = stmt->child;
            CcmmcAst *expr = stmt->child->right_sibling;
            generate_expression(expr, state, result, op1, op2);
            if (lvar->type_value == CCMMC_AST_VALUE_INT &&
                expr->type_value == CCMMC_AST_VALUE_FLOAT) {
                fprintf(state->asm_output,
                    "\tfmov\t%s, %s\n"
                    "\tfcvtas\t%s, %s\n",
                    FPREG_TMP, result,
                    result, FPREG_TMP);
            } else if (
                lvar->type_value == CCMMC_AST_VALUE_FLOAT &&
                expr->type_value == CCMMC_AST_VALUE_INT) {
                fprintf(state->asm_output,
                    "\tscvtf\t%s, %s\n"
                    "\tfmov\t%s, %s\n",
                    FPREG_TMP, result,
                    result, FPREG_TMP);
            }
            store_variable(lvar, state, result);
            ccmmc_register_unlock(state->reg_pool, tmp1);
            ccmmc_register_unlock(state->reg_pool, tmp2);
            ccmmc_register_unlock(state->reg_pool, tmp3);
            ccmmc_register_free(state->reg_pool, tmp1, &current_offset);
            ccmmc_register_free(state->reg_pool, tmp2, &current_offset);
            ccmmc_register_free(state->reg_pool, tmp3, &current_offset);
#undef FPREG_TMP
            } break;
        case CCMMC_KIND_STMT_IF:
            generate_statement(stmt->child->right_sibling,
                state, current_offset);
            if (stmt->child->right_sibling->right_sibling->type_node
                != CCMMC_AST_NODE_NUL)
                generate_statement(stmt->child->right_sibling->right_sibling,
                    state, current_offset);
            break;
        case CCMMC_KIND_STMT_FUNCTION_CALL:
            break;
        case CCMMC_KIND_STMT_RETURN:
            break;
        default:
            assert(false);
    }
}

static uint64_t generate_local_variable(
    CcmmcAst *local_decl, CcmmcState *state, uint64_t current_offset)
{
    for (CcmmcAst *var_decl = local_decl->child->right_sibling;
         var_decl != NULL; var_decl = var_decl->right_sibling) {
        CcmmcSymbol *var_sym = ccmmc_symbol_table_retrieve(state->table,
            var_decl->value_id.name);
        switch (var_decl->value_id.kind) {
            case CCMMC_KIND_ID_NORMAL:
                current_offset += 4;
                var_sym->attr.addr = current_offset;
                break;
            case CCMMC_KIND_ID_ARRAY: {
                size_t total_elements = 1;
                assert(var_sym->type.array_dimension > 0);
                for (size_t i = 0; i < var_sym->type.array_dimension; i++)
                    total_elements *= var_sym->type.array_size[i];
                current_offset += total_elements * 4;
                var_sym->attr.addr = current_offset;
                } break;
            case CCMMC_KIND_ID_WITH_INIT: {
                current_offset += 4;
                var_sym->attr.addr = current_offset;
                // TODO: initializer
                } break;
            default:
                assert(false);
        }
    }
    return current_offset;
}

static void generate_block(
    CcmmcAst *block, CcmmcState *state, uint64_t current_offset)
{
    ccmmc_symbol_table_reopen_scope(state->table);

    CcmmcAst *child = block->child;
    uint64_t orig_offset = current_offset;
    uint64_t offset_diff = 0;
    if (child != NULL && child->type_node == CCMMC_AST_NODE_VARIABLE_DECL_LIST) {
        for (CcmmcAst *local = child->child; local != NULL; local = local->right_sibling)
            current_offset = generate_local_variable(local, state, current_offset);
        offset_diff = current_offset - orig_offset;
        if (offset_diff > 0) {
            if (safe_immediate(offset_diff)) {
                fprintf(state->asm_output, "\tsub\tsp, sp, #%" PRIu64 "\n",
                    offset_diff);
            } else {
                CcmmcTmp *tmp = ccmmc_register_alloc(state->reg_pool, &current_offset);
                const char *reg_name = ccmmc_register_lock(state->reg_pool, tmp);
                fprintf(state->asm_output,
                    "\tldr\t%s, =%" PRIu64 "\n"
                    "\tsub\tsp, sp, %s\n", reg_name, offset_diff, reg_name);
                ccmmc_register_unlock(state->reg_pool, tmp);
                ccmmc_register_free(state->reg_pool, tmp, &current_offset);
            }
        }
        child = child->right_sibling;
    }
    if (child != NULL && child->type_node == CCMMC_AST_NODE_STMT_LIST) {
        for (CcmmcAst *stmt = child->child; stmt != NULL; stmt = stmt->right_sibling)
            generate_statement(stmt, state, current_offset);
    }
    if (offset_diff > 0) {
        if (safe_immediate(offset_diff)) {
            fprintf(state->asm_output, "\tadd\tsp, sp, #%" PRIu64 "\n",
                offset_diff);
        } else {
            CcmmcTmp *tmp = ccmmc_register_alloc(state->reg_pool, &current_offset);
            const char *reg_name = ccmmc_register_lock(state->reg_pool, tmp);
            fprintf(state->asm_output,
                "\tldr\t%s, =%" PRIu64 "\n"
                "\tadd\tsp, sp, %s\n", reg_name, offset_diff, reg_name);
            ccmmc_register_unlock(state->reg_pool, tmp);
            ccmmc_register_free(state->reg_pool, tmp, &current_offset);
        }
    }

    ccmmc_symbol_table_close_scope(state->table);
}

static void generate_function(CcmmcAst *function, CcmmcState *state)
{
    fputs("\t.text\n\t.align\t2\n", state->asm_output);
    fprintf(state->asm_output,
        "\t.type\t%s, %%function\n"
        "\t.global\t%s\n"
        "%s:\n"
        "\tstp\tlr, fp, [sp, -16]!\n"
        "\tmov\tfp, sp\n",
        function->child->right_sibling->value_id.name,
        function->child->right_sibling->value_id.name,
        function->child->right_sibling->value_id.name);
    CcmmcAst *param_node = function->child->right_sibling->right_sibling;
    CcmmcAst *block_node = param_node->right_sibling;
    generate_block(block_node, state, 0);
    fprintf(state->asm_output,
        "\tldp\tlr, fp, [sp], 16\n"
        "\tret\tlr\n"
        "\t.size\t%s, .-%s\n",
        function->child->right_sibling->value_id.name,
        function->child->right_sibling->value_id.name);
}

static void generate_program(CcmmcState *state)
{
    for (CcmmcAst *global_decl = state->ast->child; global_decl != NULL;
         global_decl = global_decl->right_sibling) {
        switch (global_decl->value_decl.kind) {
            case CCMMC_KIND_DECL_VARIABLE:
                generate_global_variable(global_decl, state);
                break;
            case CCMMC_KIND_DECL_FUNCTION:
                generate_function(global_decl, state);
                break;
            case CCMMC_KIND_DECL_FUNCTION_PARAMETER:
            case CCMMC_KIND_DECL_TYPE:
            default:
                assert(false);
        }
    }
}

void ccmmc_code_generation(CcmmcState *state)
{
    state->table->this_scope = NULL;
    state->table->current = NULL;
    ccmmc_symbol_table_reopen_scope(state->table);
    state->reg_pool = ccmmc_register_init(state->asm_output);
    fputs(
        "fp\t.req\tx29\n"
        "lr\t.req\tx30\n", state->asm_output);
    generate_program(state);
    ccmmc_register_fini(state->reg_pool);
    state->reg_pool = NULL;
}

// vim: set sw=4 ts=4 sts=4 et:
