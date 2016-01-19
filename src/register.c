#include "register.h"

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REG_NUM 6
#define REG_RESERVED "w9"
#define REG_LOCK_MAX 3
#define REG_SIZE 4
#define SPILL_MAX 64

static const char *reg_name[REG_NUM] = {
    "w10", "w11", "w12", "w13", "w14", "w15"};

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
    pool->spill = malloc(sizeof(CcmmcTmp*) * SPILL_MAX);
    pool->top = 0;
    pool->lock_max = REG_LOCK_MAX;
    pool->lock_cnt = 0;
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
            fprintf(pool->asm_output, "\t\tmov\t%s, %s\n", REG_RESERVED,
                pool->list[i]->name);
            fprintf(pool->asm_output, "\t\tldr\t%s, [fp, #-%" PRIu64 "]\n",
                pool->list[i]->name, tmp->addr);
            fprintf(pool->asm_output, "\t\tstr\t%s, [fp, #-%" PRIu64 "]\n",
                REG_RESERVED, tmp->addr);

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
    for (i = 0; i < pool->num && pool->list[i] != tmp->reg; i++);

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
    else {
        if (i < pool->num) {
            // pool
            pool->top--;

            // gen code to move the last tmp to this register
            fprintf(pool->asm_output, "\t\t/* ccmmc_register_free(): */\n");
            fprintf(pool->asm_output, "\t\tldr\t%s, [fp, #-%" PRIu64 "]\n",
                tmp->reg->name, pool->spill[pool->top - pool->num]->addr);
            fprintf(pool->asm_output, "\t\tadd\tsp, sp, #%d\n", REG_SIZE);

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
            // tmp is on the stack, not handled
            assert(false);
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
    for (int i = 0; i < REG_NUM; i++)
        fprintf(pool->asm_output, "\tstr\t%s, [sp, #-%d]\n",
            reg_name[i],
            (i + 1) * REG_SIZE);
    fprintf(pool->asm_output, "\tsub\tsp, sp, %d\n", REG_NUM * REG_SIZE);
}

void ccmmc_register_caller_load(CcmmcRegPool *pool)
{
    fprintf(pool->asm_output, "\tadd\tsp, sp, %d\n", REG_NUM * REG_SIZE);
    for (int i = 0; i < REG_NUM; i++)
        fprintf(pool->asm_output, "\tldr\t%s, [sp, #-%d]\n",
            reg_name[i],
            (i + 1) * REG_SIZE);
}

void ccmmc_register_fini(CcmmcRegPool *pool)
{
    // TODO: free register pool
}

// vim: set sw=4 ts=4 sts=4 et:
