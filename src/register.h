#include <inttypes.h>
#include <stdio.h>

#ifndef CCMMC_HEADER_REGISTER_H
#define CCMMC_HEADER_REGISTER_H

typedef struct CcmmcRegStruct CcmmcReg;
typedef struct CcmmcTmpStruct {
    CcmmcReg *reg;
    uint64_t addr;
} CcmmcTmp;

typedef struct CcmmcRegStruct {
    CcmmcTmp *tmp;
    int lock;
    const char *name;
} CcmmcReg;

typedef struct CcmmcRegPoolStruct {
    CcmmcReg **list;
    int num;
    int top;
    CcmmcTmp **spill;
    int lock_max;
    int lock_cnt;
    FILE *asm_output;
} CcmmcRegPool;

CcmmcRegPool    *ccmmc_register_init                (FILE *asm_output);
CcmmcTmp        *ccmmc_register_alloc               (CcmmcRegPool *pool,
                                                     uint64_t *offset);
const char      *ccmmc_register_lock                (CcmmcRegPool *pool,
                                                     CcmmcTmp *tmp);
void             ccmmc_register_unlock              (CcmmcRegPool *pool,
                                                     CcmmcTmp *tmp);
void             ccmmc_register_free                (CcmmcRegPool *pool,
                                                     CcmmcTmp *tmp,
                                                     uint64_t *offset);
void             ccmmc_register_extend_name         (CcmmcTmp *tmp, char *extend_name);
void             ccmmc_register_caller_save         (CcmmcRegPool *pool);
void             ccmmc_register_caller_load         (CcmmcRegPool *pool);
void             ccmmc_register_fini                (CcmmcRegPool *pool);

#endif
// vim: set sw=4 ts=4 sts=4 et:
