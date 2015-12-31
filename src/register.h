#ifndef CCMMC_HEADER_REGISTER_H
#define CCMMC_HEADER_REGISTER_H

typedef struct CcmmcRegStruct CcmmcReg;
typedef struct CcmmcTmpStruct {
    CcmmcReg *reg;
    int addr;
} CcmmcTmp;

typedef struct CcmmcRegStruct {
    CcmmcTmp *tmp;
    int lock;
    const char *name;
} CcmmcReg;

typedef struct CcmmcRegPoolStruct {
    CcmmcReg **list;
    int num;
    int lock_cnt;
    int lock_max;
    int top;
    int exceed;
} CcmmcRegPool;

CcmmcRegPool    *ccmmc_register_init                (void);
CcmmcTmp        *ccmmc_register_alloc               (CcmmcRegPool *pool);
const char      *ccmmc_register_lock                (CcmmcRegPool *pool,
                                                     CcmmcTmp *tmp);
void             ccmmc_register_unlock              (CcmmcRegPool *pool,
                                                     CcmmcTmp *tmp);
void             ccmmc_register_free                (CcmmcRegPool *pool,
                                                     CcmmcTmp *tmp);
void             ccmmc_register_caller_save         (CcmmcRegPool *pool);
void             ccmmc_register_caller_load         (CcmmcRegPool *pool);

#endif
// vim: set sw=4 ts=4 sts=4 et:
