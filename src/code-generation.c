#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "code-generation.h"
#include "register.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    return false; // imm <= 4096;
}

static void generate_expression(CcmmcAst *expr, CcmmcState *state,
    CcmmcTmp *result, uint64_t *current_offset);
static void calc_array_offset(CcmmcAst *ref, CcmmcSymbolType *type,
    CcmmcState *state, CcmmcTmp *result, uint64_t *current_offset)
{
    size_t i;
    CcmmcAst *index_node;
    CcmmcTmp *index, *mul;
    const char *result_reg, *index_reg, *mul_reg;

    generate_expression(ref->child, state, result, current_offset);
    index = ccmmc_register_alloc(state->reg_pool, current_offset);
    mul = ccmmc_register_alloc(state->reg_pool, current_offset);
    for (i = 1, index_node = ref->child->right_sibling; index_node != NULL;
         i++, index_node = index_node->right_sibling) {
        generate_expression(index_node, state, index, current_offset);
        result_reg = ccmmc_register_lock(state->reg_pool, result);
        index_reg = ccmmc_register_lock(state->reg_pool, index);
        mul_reg = ccmmc_register_lock(state->reg_pool, mul);
        fprintf(state->asm_output,
            "\tldr\t%s, =%zu\n"
            "\tmul\t%s, %s, %s\n"
            "\tadd\t%s, %s, %s\n",
            mul_reg, type->array_size[i],
            result_reg, result_reg, mul_reg,
            result_reg, result_reg, index_reg);
        ccmmc_register_unlock(state->reg_pool, result);
        ccmmc_register_unlock(state->reg_pool, index);
        ccmmc_register_unlock(state->reg_pool, mul);
    }
    ccmmc_register_free(state->reg_pool, index, current_offset);
    ccmmc_register_free(state->reg_pool, mul, current_offset);

    result_reg = ccmmc_register_lock(state->reg_pool, result);
    fprintf(state->asm_output,
        "\tlsl\t%s, %s, #2\n",
        result_reg, result_reg);
    ccmmc_register_unlock(state->reg_pool, result);
}

#define REG_TMP "x9"
static void load_variable(CcmmcAst *id, CcmmcState *state, CcmmcTmp *dist,
    uint64_t *current_offset)
{
    const char *dist_reg, *var_name = id->value_id.name;
    CcmmcSymbol *var_sym = ccmmc_symbol_table_retrieve(state->table, var_name);

    fprintf(state->asm_output, "\t/* var load, line %zu */\n", id->line_number);
    if (ccmmc_symbol_attr_is_global(&var_sym->attr)) {
        if (id->value_id.kind != CCMMC_KIND_ID_ARRAY) {
            dist_reg = ccmmc_register_lock(state->reg_pool, dist);
            fprintf(state->asm_output,
                "\tadrp\t" REG_TMP ", %s\n"
                "\tadd\t" REG_TMP ", " REG_TMP ", #:lo12:%s\n"
                "\tldr\t%s, [" REG_TMP "]\n", var_name, var_name, dist_reg);
            ccmmc_register_unlock(state->reg_pool, dist);
        }
        else {
            // TODO: global array
        }
    } else {
        if (id->value_id.kind != CCMMC_KIND_ID_ARRAY) {
            dist_reg = ccmmc_register_lock(state->reg_pool, dist);
            if (safe_immediate(var_sym->attr.addr)) {
                fprintf(state->asm_output,
                    "\tldr\t%s, [fp, #-%" PRIu64 "]\n",
                    dist_reg, var_sym->attr.addr);
            } else {
                fprintf(state->asm_output,
                    "\tldr\t" REG_TMP ", =%" PRIu64 "\n"
                    "\tsub\t" REG_TMP ", fp, " REG_TMP "\n"
                    "\tldr\t%s, [" REG_TMP "]\n",
                    var_sym->attr.addr, dist_reg);
            }
            ccmmc_register_unlock(state->reg_pool, dist);
        }
        else {
            CcmmcTmp *offset;
            const char *offset_reg;
            char extend[8];

            offset = ccmmc_register_alloc(state->reg_pool, current_offset);
            calc_array_offset(id, &var_sym->type, state, offset, current_offset);

            dist_reg = ccmmc_register_lock(state->reg_pool, dist);
            offset_reg = ccmmc_register_lock(state->reg_pool, offset);

            ccmmc_register_extend_name(offset, extend);
            fprintf(state->asm_output,
                "\tldr\t" REG_TMP ", =%" PRIu64 "\n"
                "\tsub\t" REG_TMP ", fp, " REG_TMP "\n"
                "\tsxtw\t%s, %s\n"
                "\tadd\t" REG_TMP ", " REG_TMP ", %s\n"
                "\tldr\t%s, [" REG_TMP "]\n",
                var_sym->attr.addr,
                extend, offset_reg,
                extend,
                dist_reg);

            ccmmc_register_unlock(state->reg_pool, dist);
            ccmmc_register_unlock(state->reg_pool, offset);
            ccmmc_register_free(state->reg_pool, offset, current_offset);
        }
    }
}

static void store_variable(CcmmcAst *id, CcmmcState *state, CcmmcTmp *src,
    uint64_t *current_offset)
{
    const char *src_reg, *var_name = id->value_id.name;
    CcmmcSymbol *var_sym = ccmmc_symbol_table_retrieve(state->table, var_name);

    fprintf(state->asm_output, "\t/* var store, line %zu */\n", id->line_number);
    if (ccmmc_symbol_attr_is_global(&var_sym->attr)) {
        if (id->value_id.kind != CCMMC_KIND_ID_ARRAY) {
            src_reg = ccmmc_register_lock(state->reg_pool, src);
            fprintf(state->asm_output,
                "\tadrp\t" REG_TMP ", %s\n"
                "\tadd\t" REG_TMP ", " REG_TMP ", #:lo12:%s\n"
                "\tstr\t%s, [" REG_TMP "]\n", var_name, var_name, src_reg);
            ccmmc_register_unlock(state->reg_pool, src);
        }
        else {
            // TODO: global array
        }
    } else {
        if (id->value_id.kind != CCMMC_KIND_ID_ARRAY) {
            src_reg = ccmmc_register_lock(state->reg_pool, src);
            if (safe_immediate(var_sym->attr.addr)) {
                fprintf(state->asm_output,
                    "\tstr\t%s, [fp, #-%" PRIu64 "]\n", src_reg, var_sym->attr.addr);
            } else {
                fprintf(state->asm_output,
                    "\tldr\t" REG_TMP ", =%" PRIu64 "\n"
                    "\tsub\t" REG_TMP ", fp, " REG_TMP "\n"
                    "\tstr\t%s, [" REG_TMP "]\n",
                    var_sym->attr.addr, src_reg);
            }
            ccmmc_register_unlock(state->reg_pool, src);
        }
        else {
            CcmmcTmp *offset;
            const char *offset_reg;
            char extend[8];

            offset = ccmmc_register_alloc(state->reg_pool, current_offset);
            calc_array_offset(id, &var_sym->type, state, offset, current_offset);

            src_reg = ccmmc_register_lock(state->reg_pool, src);
            offset_reg = ccmmc_register_lock(state->reg_pool, offset);

            ccmmc_register_extend_name(offset, extend);
            fprintf(state->asm_output,
                "\tldr\t" REG_TMP ", =%" PRIu64 "\n"
                "\tsub\t" REG_TMP ", fp, " REG_TMP "\n"
                "\tsxtw\t%s, %s\n"
                "\tadd\t" REG_TMP ", " REG_TMP ", %s\n"
                "\tstr\t%s, [" REG_TMP "]\n",
                var_sym->attr.addr,
                extend, offset_reg,
                extend,
                src_reg);

            ccmmc_register_unlock(state->reg_pool, src);
            ccmmc_register_unlock(state->reg_pool, offset);
            ccmmc_register_free(state->reg_pool, offset, current_offset);
        }
    }
}
#undef REG_TMP

static const char *call_write(CcmmcAst *id, CcmmcState *state,
    uint64_t *current_offset)
{
    CcmmcAst *arg = id->right_sibling->child;
    CcmmcTmp *dist;
    const char *dist_reg;

    if (arg->type_value == CCMMC_AST_VALUE_CONST_STRING) {
        size_t label_str = state->label_number++;
        fprintf(state->asm_output,
            "\t.section .rodata\n"
            "\t.align 2\n"
            ".LC%zu:\n"
            "\t.ascii \"%s\\000\"\n"
            "\t.text\n"
            "\tadr\tx0, .LC%zu\n",
            label_str,
            arg->value_const.const_string,
            label_str);
        return "_write_str";
    } else if (arg->type_value == CCMMC_AST_VALUE_INT) {
        dist = ccmmc_register_alloc(state->reg_pool, current_offset);
        load_variable(arg, state, dist, current_offset);
        dist_reg = ccmmc_register_lock(state->reg_pool, dist);
        fprintf(state->asm_output,
            "\tmov\tw0, %s\n",
            dist_reg);
        ccmmc_register_unlock(state->reg_pool, dist);
        ccmmc_register_free(state->reg_pool, dist, current_offset);
        return "_write_int";
    } else if (arg->type_value == CCMMC_AST_VALUE_FLOAT) {
        dist = ccmmc_register_alloc(state->reg_pool, current_offset);
        load_variable(arg, state, dist, current_offset);
        dist_reg = ccmmc_register_lock(state->reg_pool, dist);
        fprintf(state->asm_output,
            "\tfmov\ts0, %s\n",
            dist_reg);
        ccmmc_register_unlock(state->reg_pool, dist);
        ccmmc_register_free(state->reg_pool, dist, current_offset);
        return "_write_float";
    }
    abort();
}

static void call_function(CcmmcAst *id, CcmmcState *state,
    uint64_t *current_offset)
{
    const char *func_name = id->value_id.name;
    ccmmc_register_caller_save(state->reg_pool);
    if (strcmp(func_name, "write") == 0)
        func_name = call_write(id, state, current_offset);
    else if (strcmp(func_name, "read") == 0)
        func_name = "_read_int";
    else if (strcmp(func_name, "fread") == 0)
        func_name = "_read_float";
    fprintf(state->asm_output, "\tbl\t%s\n", func_name);
    ccmmc_register_caller_load(state->reg_pool);
}

static void generate_expression(CcmmcAst *expr, CcmmcState *state,
    CcmmcTmp *result, uint64_t *current_offset)
{
    const char *result_reg, *op1_reg, *op2_reg;

    if (expr->type_node == CCMMC_AST_NODE_CONST_VALUE) {
        fprintf(state->asm_output,
            "\t/* const value, line %zu */\n", expr->line_number);
        result_reg = ccmmc_register_lock(state->reg_pool, result);
        if (expr->type_value == CCMMC_AST_VALUE_INT) {
            fprintf(state->asm_output,
                "\tldr\t%s, =%d\n", result_reg, expr->value_const.const_int);
        } else if (expr->type_value == CCMMC_AST_VALUE_FLOAT) {
            fprintf(state->asm_output,
                "\tldr\t%s, .LC%zu\n"
                "\t.section .rodata\n"
                "\t.align 2\n"
                ".LC%zu:\n"
                "\t.float\t%.9g\n"
                "\t.text\n",
                result_reg, state->label_number,
                state->label_number,
                expr->value_const.const_float);
            state->label_number++;
        } else {
            assert(false);
        }
        ccmmc_register_unlock(state->reg_pool, result);
        return;
    }

    if (expr->type_node == CCMMC_AST_NODE_STMT &&
        expr->value_stmt.kind == CCMMC_KIND_STMT_FUNCTION_CALL) {
        const char *func_name = expr->child->value_id.name;
        CcmmcSymbol *func_sym = ccmmc_symbol_table_retrieve(
            state->table, func_name);
        CcmmcAstValueType func_type = func_sym->type.type_base;
        call_function(expr->child, state, current_offset);
        result_reg = ccmmc_register_lock(state->reg_pool, result);
        if (func_type == CCMMC_AST_VALUE_FLOAT)
            fprintf(state->asm_output, "\tfmov\t%s, s0\n", result_reg);
        else
            fprintf(state->asm_output, "\tmov\t%s, w0\n", result_reg);
        ccmmc_register_unlock(state->reg_pool, result);
        return ;
    }

    if (expr->type_node == CCMMC_AST_NODE_ID) {
        load_variable(expr, state, result, current_offset);
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
        generate_expression(left, state, result, current_offset);
        CcmmcTmp *op1 = ccmmc_register_alloc(state->reg_pool, current_offset);
        result_reg = ccmmc_register_lock(state->reg_pool, result);
        op1_reg = ccmmc_register_lock(state->reg_pool, op1);
        fprintf(state->asm_output,
            "\t/* mov op1_reg, result_reg */\n"
            "\tmov\t%s, %s\n",
            op1_reg, result_reg);
        ccmmc_register_unlock(state->reg_pool, result);
        ccmmc_register_unlock(state->reg_pool, op1);

        if (expr->value_expr.op_binary == CCMMC_KIND_OP_BINARY_AND) {
            // logical AND needs short-curcuit evaluation
            label_short = state->label_number++;
            op1_reg = ccmmc_register_lock(state->reg_pool, op1);
            if (left->type_value == CCMMC_AST_VALUE_FLOAT)
                fprintf(state->asm_output,
                    "\tfmov\t%s, %s\n"
                    "\tfcmp\t%s, #0.0\n"
                    "\tb.eq\t.LC%zu\n",
                    FPREG_OP1, op1_reg,
                    FPREG_OP1,
                    label_short);
            else
                fprintf(state->asm_output,
                    "\tcbz\t%s, .LC%zu\n",
                    op1_reg, label_short);
            ccmmc_register_unlock(state->reg_pool, op1);
        } else if (expr->value_expr.op_binary == CCMMC_KIND_OP_BINARY_OR) {
            // logical OR needs short-curcuit evaluation
            label_short = state->label_number++;
            op1_reg = ccmmc_register_lock(state->reg_pool, op1);
            if (left->type_value == CCMMC_AST_VALUE_FLOAT)
                fprintf(state->asm_output,
                    "\tfmov\t%s, %s\n"
                    "\tfcmp\t%s, #0.0\n"
                    "\tb.ne\t.LC%zu\n",
                    FPREG_OP1, op1_reg,
                    FPREG_OP1,
                    label_short);
            else
                fprintf(state->asm_output,
                    "\tcbnz\t%s, .LC%zu\n",
                    op1_reg, label_short);
            ccmmc_register_unlock(state->reg_pool, op1);
        }

        // evaluate the right expression
        generate_expression(right, state, result, current_offset);
        CcmmcTmp *op2 = ccmmc_register_alloc(state->reg_pool, current_offset);
        result_reg = ccmmc_register_lock(state->reg_pool, result);
        op2_reg = ccmmc_register_lock(state->reg_pool, op2);
        fprintf(state->asm_output,
            "\t/* mov op2_reg, result_reg */\n"
            "\tmov\t%s, %s\n",
            op2_reg, result_reg);
        ccmmc_register_unlock(state->reg_pool, result);
        ccmmc_register_unlock(state->reg_pool, op2);

        if (left->type_value == CCMMC_AST_VALUE_FLOAT ||
            right->type_value == CCMMC_AST_VALUE_FLOAT) {
            op1_reg = ccmmc_register_lock(state->reg_pool, op1);
            if (left->type_value == CCMMC_AST_VALUE_INT)
                fprintf(state->asm_output, "\tscvtf\t%s, %s\n", FPREG_OP1, op1_reg);
            else if (left->type_value == CCMMC_AST_VALUE_FLOAT)
                fprintf(state->asm_output, "\tfmov\t%s, %s\n", FPREG_OP1, op1_reg);
            else
                assert(false);
            ccmmc_register_unlock(state->reg_pool, op1);

            op2_reg = ccmmc_register_lock(state->reg_pool, op2);
            if (right->type_value == CCMMC_AST_VALUE_INT)
                fprintf(state->asm_output, "\tscvtf\t%s, %s\n", FPREG_OP2, op2_reg);
            else if (right->type_value == CCMMC_AST_VALUE_FLOAT)
                fprintf(state->asm_output, "\tfmov\t%s, %s\n", FPREG_OP2, op2_reg);
            else
                assert(false);
            ccmmc_register_unlock(state->reg_pool, op2);
        }

        result_reg = ccmmc_register_lock(state->reg_pool, result);
        op1_reg = ccmmc_register_lock(state->reg_pool, op1);
        op2_reg = ccmmc_register_lock(state->reg_pool, op2);

        switch (expr->value_expr.op_binary) {
            case CCMMC_KIND_OP_BINARY_ADD:
                if (expr->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfadd\t%s, %s, %s\n",
                        FPREG_RESULT, FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tadd\t%s, %s, %s\n",
                        result_reg, op1_reg, op2_reg);
                break;
            case CCMMC_KIND_OP_BINARY_SUB:
                if (expr->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfsub\t%s, %s, %s\n",
                        FPREG_RESULT, FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tsub\t%s, %s, %s\n",
                        result_reg, op1_reg, op2_reg);
                break;
            case CCMMC_KIND_OP_BINARY_MUL:
                if (expr->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfmul\t%s, %s, %s\n",
                        FPREG_RESULT, FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tmul\t%s, %s, %s\n",
                        result_reg, op1_reg, op2_reg);
                break;
            case CCMMC_KIND_OP_BINARY_DIV:
                if (expr->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfdiv\t%s, %s, %s\n",
                        FPREG_RESULT, FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tsdiv\t%s, %s, %s\n",
                        result_reg, op1_reg, op2_reg);
                break;
            case CCMMC_KIND_OP_BINARY_EQ:
                if (left->type_value == CCMMC_AST_VALUE_FLOAT ||
                    right->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfcmp\t%s, %s\n", FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tcmp\t%s, %s\n", op1_reg, op2_reg);
                fprintf(state->asm_output, "\tcset\t%s, eq\n", result_reg);
                break;
            case CCMMC_KIND_OP_BINARY_GE:
                if (left->type_value == CCMMC_AST_VALUE_FLOAT ||
                    right->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfcmp\t%s, %s\n", FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tcmp\t%s, %s\n", op1_reg, op2_reg);
                fprintf(state->asm_output, "\tcset\t%s, ge\n", result_reg);
                break;
            case CCMMC_KIND_OP_BINARY_LE:
                if (left->type_value == CCMMC_AST_VALUE_FLOAT ||
                    right->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfcmp\t%s, %s\n", FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tcmp\t%s, %s\n", op1_reg, op2_reg);
                fprintf(state->asm_output, "\tcset\t%s, le\n", result_reg);
                break;
            case CCMMC_KIND_OP_BINARY_NE:
                if (left->type_value == CCMMC_AST_VALUE_FLOAT ||
                    right->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfcmp\t%s, %s\n", FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tcmp\t%s, %s\n", op1_reg, op2_reg);
                fprintf(state->asm_output, "\tcset\t%s, ne\n", result_reg);
                break;
            case CCMMC_KIND_OP_BINARY_GT:
                if (left->type_value == CCMMC_AST_VALUE_FLOAT ||
                    right->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfcmp\t%s, %s\n", FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tcmp\t%s, %s\n", op1_reg, op2_reg);
                fprintf(state->asm_output, "\tcset\t%s, gt\n", result_reg);
                break;
            case CCMMC_KIND_OP_BINARY_LT:
                if (left->type_value == CCMMC_AST_VALUE_FLOAT ||
                    right->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfcmp\t%s, %s\n", FPREG_OP1, FPREG_OP2);
                else
                    fprintf(state->asm_output, "\tcmp\t%s, %s\n", op1_reg, op2_reg);
                fprintf(state->asm_output, "\tcset\t%s, lt\n", result_reg);
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
                        result_reg,
                        label_exit,
                        label_short,
                        result_reg,
                        label_exit);
                else
                    fprintf(state->asm_output,
                        "\tcbz\t%s, .LC%zu\n"
                        "\tmov\t%s, #1\n"
                        "\tb\t.LC%zu\n"
                        ".LC%zu:\n"
                        "\tmov\t%s, #0\n"
                        ".LC%zu:\n",
                        op2_reg, label_short,
                        result_reg,
                        label_exit,
                        label_short,
                        result_reg,
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
                        result_reg,
                        label_exit,
                        label_short,
                        result_reg,
                        label_exit);
                else
                    fprintf(state->asm_output,
                        "\tcbnz\t%s, .LC%zu\n"
                        "\tmov\t%s, #0\n"
                        "\tb\t.LC%zu\n"
                        ".LC%zu:\n"
                        "\tmov\t%s, #1\n"
                        ".LC%zu:\n",
                        op2_reg, label_short,
                        result_reg,
                        label_exit,
                        label_short,
                        result_reg,
                        label_exit);
                } break;
            default:
                assert(false);
        }
        ccmmc_register_unlock(state->reg_pool, result);
        ccmmc_register_unlock(state->reg_pool, op1);
        ccmmc_register_unlock(state->reg_pool, op2);

        ccmmc_register_free(state->reg_pool, op1, current_offset);
        ccmmc_register_free(state->reg_pool, op2, current_offset);

        if (expr->type_value == CCMMC_AST_VALUE_FLOAT) {
            result_reg = ccmmc_register_lock(state->reg_pool, result);
            fprintf(state->asm_output, "\tfmov\t%s, %s\n", result_reg, FPREG_RESULT);
            ccmmc_register_unlock(state->reg_pool, result);
        }
        return;
    }

    if (expr->value_expr.kind == CCMMC_KIND_EXPR_UNARY_OP) {
        CcmmcAst *arg = expr->child;
        fprintf(state->asm_output,
            "\t/* expr unary op, line %zu */\n", expr->line_number);
        generate_expression(arg, state, result, current_offset);
        CcmmcTmp *op1 = ccmmc_register_alloc(state->reg_pool, current_offset);
        result_reg = ccmmc_register_lock(state->reg_pool, result);
        op1_reg = ccmmc_register_lock(state->reg_pool, op1);
        fprintf(state->asm_output,
            "\t/* mov op1_reg, result_reg */\n"
            "\tmov\t%s, %s\n",
            op1_reg, result_reg);
        ccmmc_register_unlock(state->reg_pool, result);
        ccmmc_register_unlock(state->reg_pool, op1);

        if (arg->type_value == CCMMC_AST_VALUE_FLOAT) {
            op1_reg = ccmmc_register_lock(state->reg_pool, op1);
            fprintf(state->asm_output, "\tfmov\t%s, %s\n", FPREG_OP1, op1_reg);
            ccmmc_register_unlock(state->reg_pool, op1);
        }

        result_reg = ccmmc_register_lock(state->reg_pool, result);
        op1_reg = ccmmc_register_lock(state->reg_pool, op1);
        switch (expr->value_expr.op_unary) {
            case CCMMC_KIND_OP_UNARY_POSITIVE:
                fputs("\t/* nop */\n", state->asm_output);
                break;
            case CCMMC_KIND_OP_UNARY_NEGATIVE:
                if (arg->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfneg\t%s, %s\n", FPREG_RESULT, FPREG_OP1);
                else
                    fprintf(state->asm_output, "\tneg\t%s, %s\n", result_reg, op1_reg);
                break;
            case CCMMC_KIND_OP_UNARY_LOGICAL_NEGATION:
                if (arg->type_value == CCMMC_AST_VALUE_FLOAT)
                    fprintf(state->asm_output, "\tfcmp\t%s, #0.0\n", FPREG_OP1);
                else
                    fprintf(state->asm_output, "\tcmp\t%s, wzr\n", op1_reg);
                fprintf(state->asm_output, "\tcset\t%s, eq\n", result_reg);
                break;
            default:
                assert(false);
        }
        ccmmc_register_unlock(state->reg_pool, result);
        ccmmc_register_unlock(state->reg_pool, op1);

        ccmmc_register_free(state->reg_pool, op1, current_offset);

        if (expr->type_value == CCMMC_AST_VALUE_FLOAT) {
            result_reg = ccmmc_register_lock(state->reg_pool, result);
            fprintf(state->asm_output, "\tfmov\t%s, %s\n", result_reg, FPREG_RESULT);
            ccmmc_register_unlock(state->reg_pool, result);
        }
        return;
    }

#undef FPREG_RESULT
#undef FPREG_OP1
#undef FPREG_OP2

    assert(false);
}

static void calc_expression_result(CcmmcAst *expr, CcmmcAstValueType type,
    CcmmcState *state, CcmmcTmp *result, uint64_t *current_offset)
{
#define FPREG_TMP  "s16"
    const char *result_reg;
    generate_expression(expr, state, result, current_offset);

    result_reg = ccmmc_register_lock(state->reg_pool, result);
    if (expr->type_value == CCMMC_AST_VALUE_FLOAT &&
        type == CCMMC_AST_VALUE_INT) {
        fprintf(state->asm_output,
            "\tfmov\t%s, %s\n"
            "\tfcvtas\t%s, %s\n",
            FPREG_TMP, result_reg,
            result_reg, FPREG_TMP);
    } else if (expr->type_value == CCMMC_AST_VALUE_INT &&
        type == CCMMC_AST_VALUE_FLOAT) {
        fprintf(state->asm_output,
            "\tscvtf\t%s, %s\n"
            "\tfmov\t%s, %s\n",
            FPREG_TMP, result_reg,
            result_reg, FPREG_TMP);
    }
    ccmmc_register_unlock(state->reg_pool, result);
#undef FPREG_TMP
}

static void calc_and_save_expression_result(CcmmcAst *lvar, CcmmcAst *expr,
    CcmmcState *state, uint64_t *current_offset)
{
    CcmmcTmp *result;
    result = ccmmc_register_alloc(state->reg_pool, current_offset);
    calc_expression_result(expr, lvar->type_value, state, result, current_offset);
    store_variable(lvar, state, result, current_offset);
    ccmmc_register_free(state->reg_pool, result, current_offset);
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
        case CCMMC_KIND_STMT_WHILE: {
#define FPREG_TMP  "s16"
            size_t label_cmp = state->label_number++;
            size_t label_exit = state->label_number++;
            const char *result_reg;
            CcmmcTmp *result = ccmmc_register_alloc(state->reg_pool, &current_offset);

            // while condition
            fprintf(state->asm_output, ".LC%zu:\n", label_cmp);
            generate_expression(stmt->child, state, result, &current_offset);
            result_reg = ccmmc_register_lock(state->reg_pool, result);
            if (stmt->child->type_value == CCMMC_AST_VALUE_FLOAT)
                fprintf(state->asm_output,
                    "\tfmov\t%s, %s\n"
                    "\tfcmp\t%s, #0.0\n"
                    "\tb.e\t.LC%zu\n",
                    FPREG_TMP,
                    result_reg,
                    FPREG_TMP,
                    label_exit);
            else
                fprintf(state->asm_output,
                    "\tcbz\t%s, .LC%zu\n",
                    result_reg,
                    label_exit);
            ccmmc_register_unlock(state->reg_pool, result);
            ccmmc_register_free(state->reg_pool, result, &current_offset);

            // while body
            generate_statement(stmt->child->right_sibling,
                state, current_offset);
            fprintf(state->asm_output,
                "\tb\t.LC%zu\n"
                ".LC%zu:\n",
                label_cmp,
                label_exit);
#undef FPREG_TMP
            } break;
        case CCMMC_KIND_STMT_FOR:
            break;
        case CCMMC_KIND_STMT_ASSIGN:
            calc_and_save_expression_result(stmt->child,
                stmt->child->right_sibling, state, &current_offset);
            break;
        case CCMMC_KIND_STMT_IF: {
#define FPREG_TMP  "s16"
            size_t label_cross_if = state->label_number++;
            const char *result_reg;
            CcmmcTmp *result = ccmmc_register_alloc(state->reg_pool, &current_offset);

            // if condition
            generate_expression(stmt->child, state, result, &current_offset);
            result_reg = ccmmc_register_lock(state->reg_pool, result);
            if (stmt->child->type_value == CCMMC_AST_VALUE_FLOAT)
                fprintf(state->asm_output,
                    "\tfmov\t%s, %s\n"
                    "\tfcmp\t%s, #0.0\n"
                    "\tb.e\t.LC%zu\n",
                    FPREG_TMP,
                    result_reg,
                    FPREG_TMP,
                    label_cross_if);
            else
                fprintf(state->asm_output,
                    "\tcbz\t%s, .LC%zu\n",
                    result_reg,
                    label_cross_if);
            ccmmc_register_unlock(state->reg_pool, result);
            ccmmc_register_free(state->reg_pool, result, &current_offset);

            // if body
            generate_statement(stmt->child->right_sibling,
                state, current_offset);

            if (stmt->child->right_sibling->right_sibling->type_node
                == CCMMC_AST_NODE_NUL) {
                // no else
                fprintf(state->asm_output, ".LC%zu:\n", label_cross_if);
            }
            else {
                // jump across else
                size_t label_exit = state->label_number++;
                fprintf(state->asm_output,
                    "\tb\t.LC%zu\n"
                    ".LC%zu:\n",
                    label_exit,
                    label_cross_if);

                // else body
                generate_statement(stmt->child->right_sibling->right_sibling,
                    state, current_offset);
                fprintf(state->asm_output, ".LC%zu:\n", label_exit);
#undef FPREG_TMP
            }
            } break;
        case CCMMC_KIND_STMT_FUNCTION_CALL:
            call_function(stmt->child, state, &current_offset);
            break;
        case CCMMC_KIND_STMT_RETURN:
            for (CcmmcAst *func = stmt->parent; ; func = func->parent) {
                if (func->type_node == CCMMC_AST_NODE_DECL &&
                    func->value_decl.kind == CCMMC_KIND_DECL_FUNCTION) {
                    const char *func_name = func->child->right_sibling->value_id.name;
                    CcmmcSymbol *func_sym =
                        ccmmc_symbol_table_retrieve(state->table, func_name);
                    CcmmcAstValueType func_type = func_sym->type.type_base;
                    const char *result_reg;
                    CcmmcTmp *result;

                    result = ccmmc_register_alloc(state->reg_pool, &current_offset);
                    calc_expression_result(stmt->child, func_sym->type.type_base,
                        state, result, &current_offset);
                    result_reg = ccmmc_register_lock(state->reg_pool, result);
                    if (func_type == CCMMC_AST_VALUE_FLOAT)
                        fprintf(state->asm_output, "\tfmov\ts0, %s\n", result_reg);
                    else
                        fprintf(state->asm_output, "\tmov\tw0, %s\n", result_reg);
                    ccmmc_register_unlock(state->reg_pool, result);
                    ccmmc_register_free(state->reg_pool, result, &current_offset);

                    // XXX: We should fix the location of the return label
                    // instead of copying code and modifying sp here.
                    if (safe_immediate(current_offset)) {
                        fprintf(state->asm_output, "\tadd\tsp, sp, #%" PRIu64 "\n",
                            current_offset);
                    } else {
#define REG_TMP "x9"
                        fprintf(state->asm_output,
                            "\tldr\t%s, =%" PRIu64 "\n"
                            "\tadd\tsp, sp, %s\n", REG_TMP, current_offset, REG_TMP);
#undef REG_TMP
                    }
                    fprintf(state->asm_output, "\tb\t.LR_%s\n", func_name);
                    break;
                }
            }
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
                // FIXME: This only works for single constant initializer.
                // The value of sp is wrong, so evaluating expressions
                // can overwrite other variables!
                calc_and_save_expression_result(var_decl, var_decl->child,
                    state, &current_offset);
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
#define REG_TMP "x9"
                fprintf(state->asm_output,
                    "\tldr\t%s, =%" PRIu64 "\n"
                    "\tsub\tsp, sp, %s\n", REG_TMP, offset_diff, REG_TMP);
#undef REG_TMP
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
#define REG_TMP "x9"
            fprintf(state->asm_output,
                "\tldr\t%s, =%" PRIu64 "\n"
                "\tadd\tsp, sp, %s\n", REG_TMP, offset_diff, REG_TMP);
#undef REG_TMP
        }
    }

    ccmmc_symbol_table_close_scope(state->table);
}

static void generate_function(CcmmcAst *function, CcmmcState *state)
{
    fputs("\t.text\n\t.align\t2\n", state->asm_output);
    const char *func_name = function->child->right_sibling->value_id.name;
    const char *symbol_name = func_name;
    // XXX: We have to rewrite some names to workaround TA's broken toolchain
    if (strcmp(func_name, "main") == 0 || strcmp(func_name, "MAIN") == 0)
        symbol_name = "_start_MAIN";
    fprintf(state->asm_output,
        "\t.type\t%s, %%function\n"
        "\t.global\t%s\n"
        "%s:\n"
        "\tstp\tlr, fp, [sp, -16]!\n"
        "\tmov\tfp, sp\n",
        symbol_name,
        symbol_name,
        symbol_name);
    CcmmcAst *param_node = function->child->right_sibling->right_sibling;
    CcmmcAst *block_node = param_node->right_sibling;
    generate_block(block_node, state, 0);
    fprintf(state->asm_output,
        ".LR_%s:\n"
        "\tldp\tlr, fp, [sp], 16\n"
        "\tret\tlr\n"
        "\t.size\t%s, .-%s\n",
        func_name,
        symbol_name,
        symbol_name);
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
            case CCMMC_KIND_DECL_TYPE:
                break;
            case CCMMC_KIND_DECL_FUNCTION_PARAMETER:
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
