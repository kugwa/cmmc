#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "code-generation.h"

#include <assert.h>
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
                fprintf(state->asm_output, "\t.size\t%s, 4\n", var_sym->name);
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
                    fprintf(state->asm_output, "%s:\n\t.word\t%d\n",
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
                    fprintf(state->asm_output, "%s:\n\t.float\t%.9g\n",
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

static void generate_function(CcmmcAst *funcion, CcmmcState *state)
{
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
    generate_program(state);
}

// vim: set sw=4 ts=4 sts=4 et:
