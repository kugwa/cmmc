#include "register.h"

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REG_NUM 15
// #define REG_NUM 5
#define REG_ADDR "x9"
#define REG_SWAP "w10"
#define SPILL_MAX 32
#define REG_LOCK_MAX 3
#define REG_SIZE 4

static const char *reg_name[REG_NUM] = {
    "w11", "w12", "w13", "w14", "w15", "w19", "w20", "w21", "w22", "w23", "w24", "w25", "w26", "w27", "w28"};
    // "w11", "w12", "w13", "w14", "w15"};

CcmmcRegPool *ccmmc_register_init(FILE *asm_output)
{
    CcmmcRegPool *pool = malloc(sizeof(CcmmcRegPool));
    pool->num = REG_NUM;
    pool->list = malloc(sizeof(CcmmcReg*) * pool->num);
    for (int i = 0; i < pool->num; i++) {
        pool->list[i] = malloc(sizeof(CcmmcReg));
        pool->list[i]->tmp = NULL;
        pool->list[i]->lock = false;
        pool->list[i]->name = reg_name[i];
    }
    pool->spill_max = SPILL_MAX;
    pool->spill = malloc(sizeof(CcmmcTmp*) * pool->spill_max);
    pool->top = 0;
    pool->lock_max = REG_LOCK_MAX;
    pool->lock_cnt = 0;
    pool->print_buf = malloc(sizeof(char) * ((REG_NUM + 1) * 25));
    pool->asm_output = asm_output;
    return pool;
}

CcmmcTmp *ccmmc_register_alloc(CcmmcRegPool *pool, uint64_t *offset)
{
    CcmmcTmp *tmp = malloc(sizeof(CcmmcTmp));
    if (pool->top < pool->num) {
        // tmp
        tmp->addr = 0;
        tmp->reg = pool->list[pool->top];

        // reg
        tmp->reg->tmp = tmp;
        tmp->reg->lock = false;

        // pool
        pool->top++;
    }
    else {
        // gen code to alloc space on the stack
        fprintf(pool->asm_output,
            "\t\t/* ccmmc_register_alloc(): */\n"
            "\t\tsub\tsp, sp, #%d\n",
            REG_SIZE);

        // tmp and offset
        *offset += REG_SIZE;
        tmp->addr = *offset;
        tmp->reg = NULL;

        // spill
        if (pool->top - pool->num >= pool->spill_max) {
            pool->spill_max *= 2;
            pool->spill = realloc(pool->spill, sizeof(CcmmcTmp*) * pool->spill_max);
        }
        pool->spill[pool->top - pool->num] = tmp;

        // pool
        pool->top++;
    }
    return tmp;
}

const char *ccmmc_register_lock(CcmmcRegPool *pool, CcmmcTmp *tmp)
{
    const char *reg = NULL;
    if (pool->lock_cnt < pool->lock_max) {
        if (tmp->reg !=NULL) {
            if (!tmp->reg->lock) {
                // reg
                tmp->reg->lock = true;

                // pool
                pool->lock_cnt++;
            }
            reg = tmp->reg->name;
        }
        else {
            // find a unlocked reg
            int i, j;
            for (i = 0; i < pool->num && pool->list[i]->lock; i++);
            assert(i < pool->num); //must found

            // gen code to swap the tmp in the register and the tmp on the stack
            fprintf(pool->asm_output,
                "\t\t/* ccmmc_register_lock(): swap %s, [fp, #-%" PRIu64 "] */\n",
                pool->list[i]->name,
                tmp->addr);
            fprintf(pool->asm_output, // REG_ADDR holds the address on the stack
                "\t\tldr\t" REG_ADDR ", =%" PRIu64 "\n"
                "\t\tsub\t" REG_ADDR ", fp, " REG_ADDR "\n"
                "\t\tmov\t" REG_SWAP ", %s\n"
                "\t\tldr\t%s, [" REG_ADDR "]\n"
                "\t\tstr\t" REG_SWAP ", [" REG_ADDR "]\n",
                tmp->addr,
                pool->list[i]->name,
                pool->list[i]->name);

            // find the index of tmp in pool->spill
            for (j = 0; j < pool->top - pool->num && pool->spill[j] != tmp; j++);

            // spill
            pool->spill[j] = pool->list[i]->tmp;

            // old tmp of the reg
            pool->list[i]->tmp->reg = NULL;
            pool->list[i]->tmp->addr = tmp->addr;

            // tmp
            tmp->reg = pool->list[i];
            tmp->addr = 0;

            // register
            tmp->reg->tmp = tmp;
            tmp->reg->lock = true;

            // pool
            pool->lock_cnt++;

            reg = tmp->reg->name;
        }
    }
    return reg;
}

void ccmmc_register_unlock(CcmmcRegPool *pool, CcmmcTmp *tmp)
{
    if (tmp->reg != NULL && tmp->reg->lock) {
        // reg
        tmp->reg->lock = false;

        // pool
        pool->lock_cnt--;
    }
}

void ccmmc_register_free(CcmmcRegPool *pool, CcmmcTmp *tmp, uint64_t *offset)
{
    // find the index of register associated with tmp
    int i;
    for (i = 0; i < pool->num && pool->list[i]->tmp != tmp; i++);
    if (pool->top <= pool->num) {
        // tmp
        free(tmp);

        // pool
        pool->top--;
        assert(i < pool->num); //must found
        if (i < pool->top) {
            CcmmcReg *swap = pool->list[i];
            pool->list[i] = pool->list[pool->top];
            pool->list[pool->top] = swap;
        }
    }
    else if (i < pool->num) {
        // pool
        pool->top--;

        // gen code to move the last tmp to this register
        fprintf(pool->asm_output,
            "\t\t/* ccmmc_register_free(): mov %s, [fp, #-%" PRIu64 "] */\n",
            tmp->reg->name, pool->spill[pool->top - pool->num]->addr);
        fprintf(pool->asm_output, // REG_ADDR holds the address on the stack
            "\t\tldr\t" REG_ADDR ", =%" PRIu64 "\n"
            "\t\tsub\t" REG_ADDR ", fp, " REG_ADDR "\n"
            "\t\tldr\t%s, [" REG_ADDR "]\n"
            "\t\tadd\tsp, sp, #%d\n",
            pool->spill[pool->top - pool->num]->addr,
            tmp->reg->name,
            REG_SIZE);

        // offset
        *offset -= REG_SIZE;

        // reg
        tmp->reg->tmp = pool->spill[pool->top - pool->num];

        // the last tmp
        tmp->reg->tmp->reg = tmp->reg;
        tmp->reg->tmp->addr = 0;

        // tmp
        free(tmp);
    }
    else {
        for (i = 0; i < pool->top - pool->num && pool->spill[i] != tmp; i++);
        assert(i < pool->top - pool->num); //must found

        // pool
        pool->top--;

        if (i < pool->top - pool->num) {
            // gen code to move the last tmp to the hole
            fprintf(pool->asm_output,
                "\t\t/* ccmmc_register_free(): "
                "mov [fp, #-%" PRIu64 "] , [fp, #-%" PRIu64 "] */\n",
                pool->spill[i]->addr,
                pool->spill[pool->top - pool->num]->addr);
            fprintf(pool->asm_output,
                "\t\tldr\t" REG_ADDR ", =%" PRIu64 "\n"
                "\t\tsub\t" REG_ADDR ", fp, " REG_ADDR "\n"
                "\t\tldr\t" REG_SWAP ", [" REG_ADDR "]\n"
                "\t\tldr\t" REG_ADDR ", =%" PRIu64 "\n"
                "\t\tsub\t" REG_ADDR ", fp, " REG_ADDR "\n"
                "\t\tstr\t" REG_SWAP ", [" REG_ADDR "]\n"
                "\t\tadd\tsp, sp, #%d\n",
                pool->spill[pool->top - pool->num]->addr,
                pool->spill[i]->addr,
                REG_SIZE);

            // offset
            *offset -= REG_SIZE;

            // spill
            pool->spill[pool->top - pool->num]->addr = pool->spill[i]->addr;
            free(pool->spill[i]);
            pool->spill[i] = pool->spill[pool->top - pool->num];
        }
        else {
            // gen code to remove the last tmp
            fprintf(pool->asm_output,
                "\t\t/* ccmmc_register_free(): */\n"
                "\t\tadd\tsp, sp, #%d\n", REG_SIZE);

            // offset
            *offset -= REG_SIZE;

            // spill
            free(pool->spill[i]);
        }
    }
}

void ccmmc_register_extend_name(CcmmcTmp *tmp, char *extend_name)
{
    if (tmp->reg != NULL) {
        strcpy(extend_name, tmp->reg->name);
        extend_name[0] = 'x';
    }
    else extend_name[0] = '\0';
}

void ccmmc_register_caller_save(CcmmcRegPool *pool)
{
    char buf[30];
    int bound = pool->num;
    if (pool->top < bound)
        bound = pool->top;

    for (int i = 0; i < bound; i++)
        fprintf(pool->asm_output, "\tstr\t%s, [sp, #-%d]\n",
            pool->list[i]->name,
            (i + 1) * REG_SIZE);
    if (bound > 0)
        fprintf(pool->asm_output, "\tsub\tsp, sp, %d\n", bound * REG_SIZE);

    if (bound > 0)
        sprintf(pool->print_buf, "\tadd\tsp, sp, %d\n", bound * REG_SIZE);
    else
        pool->print_buf[0] = '\0';
    for (int i = 0; i < bound; i++) {
        sprintf(buf, "\tldr\t%s, [sp, #-%d]\n",
            pool->list[i]->name,
            (i + 1) * REG_SIZE);
        strcat(pool->print_buf, buf);
    }
}

void ccmmc_register_caller_load(CcmmcRegPool *pool)
{
    fputs(pool->print_buf, pool->asm_output);
}

void ccmmc_register_save_arguments(CcmmcRegPool *pool, int arg_count)
{
    if (arg_count <= 0)
        return;
    if (arg_count >= 8)
        arg_count = 8;
    for (int i = 0; i < arg_count; i++)
        fprintf(pool->asm_output, "\tstr\tx%d, [sp, #-%d]\n",
            i, (i + 1) * 8);
    fprintf(pool->asm_output, "\tsub\tsp, sp, %d\n", arg_count * 8);
}

void ccmmc_register_load_arguments(CcmmcRegPool *pool, int arg_count)
{
    if (arg_count <= 0)
        return;
    if (arg_count >= 8)
        arg_count = 8;
    fprintf(pool->asm_output, "\tadd\tsp, sp, %d\n", arg_count * 8);
    for (int i = 0; i < arg_count; i++)
        fprintf(pool->asm_output, "\tldr\tx%d, [sp, #-%d]\n",
            i, (i + 1) * 8);
}

void ccmmc_register_fini(CcmmcRegPool *pool)
{
    // TODO: free register pool
}

// vim: set sw=4 ts=4 sts=4 et:
