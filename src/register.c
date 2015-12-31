#include "register.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#define REG_NUM 6
#define REG_RESERVED "x9"
#define REG_LOCK_MAX 3

static const char *reg_name[REG_NUM] = {
    "x10", "x11", "x12", "x13", "x14", "x15"};

/* static void mov(CcmmcTmp *a, CcmmcTmp *b)
{
} */

CcmmcRegPool *ccmmc_register_init(void)
{
    CcmmcRegPool *pool = malloc(sizeof(CcmmcRegPool));
    pool->num = REG_NUM;
    pool->list = malloc(sizeof(CcmmcReg*) * pool->num);
    for (int i = 0; i < pool->num; i++) {
        pool->list[i] = malloc(sizeof(CcmmcReg));
        pool->list[i]->tmp = NULL;
        pool->list[i]->lock = 0;
        pool->list[i]->name = reg_name[i];
    }
    pool->lock_max = REG_LOCK_MAX;
    pool->lock_cnt = 0;
    pool->top = 0;
    pool->exceed = 0;
    return pool;
}

CcmmcTmp *ccmmc_register_alloc(CcmmcRegPool *pool)
{
    CcmmcTmp *tmp = malloc(sizeof(CcmmcTmp));
    if (pool->top < pool->num) {
        tmp->reg = pool->list[pool->top];
        tmp->addr = 0;
        tmp->reg->tmp = tmp;
        pool->top++;
    }
    else {
        tmp->reg = NULL;
        tmp->addr = (pool->exceed + 1) * (-8); // TODO: assign an offset for original tmp
        pool->exceed++;
    }
    return tmp;
}

const char *ccmmc_register_lock(CcmmcRegPool *pool, CcmmcTmp *tmp)
{
    const char *reg = NULL;
    if (pool->lock_cnt < pool->lock_max) {
        if (tmp->reg !=NULL) {
            if (tmp->reg->lock == 0) {
                tmp->reg->lock = 1;
                pool->lock_cnt++;
            }
            reg = tmp->reg->name;
        }
        else { // find a unlocked reg
            int i;
            for (i = 0; i < pool->num && pool->list[i]->lock == 1; i++);

            // TODO: mov REG_RESERVED, pool->list[i]->name
            // TODO: ldr pool->list[i]->name, tmp->addr
            // TODO: str REG_RESERVED, tmp->addr

            pool->list[i]->tmp->reg = NULL;
            pool->list[i]->tmp->addr = tmp->addr;
            tmp->reg = pool->list[i];
            tmp->addr = 0;

            tmp->reg->lock = 1;
            pool->lock_cnt++;
            reg = tmp->reg->name;
        }
    }
    return reg;
}

void ccmmc_register_unlock(CcmmcRegPool *pool, CcmmcTmp *tmp)
{
    if (tmp->reg != NULL && tmp->reg->lock == 1) {
        tmp->reg->lock = 0;
        pool->lock_cnt--;
    }
}

void ccmmc_register_free(CcmmcRegPool *pool, CcmmcTmp *tmp)
{
    if (pool->exceed == 0) {
        int i;
        for (i = 0; i < pool->num && pool->list[i] != tmp->reg; i++);
        if (i < pool->top - 1) {
            CcmmcReg *swap = pool->list[i];
            pool->list[i] = pool->list[pool->top - 1];
            pool->list[pool->top - 1] = swap;
        }
        pool->top--;
        free(tmp);
    }
    else {
        assert(false);
    }
}

void ccmmc_register_caller_save(CcmmcRegPool *pool)
{
}

void ccmmc_register_caller_load(CcmmcRegPool *pool)
{
}

void ccmmc_register_fini(CcmmcRegPool *pool)
{
    // TODO: free register pool
}
