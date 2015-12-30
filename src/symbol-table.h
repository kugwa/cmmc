#ifndef CCMMC_HEADER_SYMBOL_TABLE_H
#define CCMMC_HEADER_SYMBOL_TABLE_H

#include "ast.h"

#include <stdbool.h>
#include <stdint.h>

#define CCMMC_SYMBOL_SCOPE_HASH_TABLE_SIZE 256

typedef enum CcmmcSymbolKind_enum {
    CCMMC_SYMBOL_KIND_TYPE,
    CCMMC_SYMBOL_KIND_VARIABLE,
    CCMMC_SYMBOL_KIND_FUNCTION
} CcmmcSymbolKind;

typedef struct CcmmcSymbolType_struct CcmmcSymbolType;
typedef struct CcmmcSymbolType_struct {
    CcmmcAstValueType type_base;
    size_t array_dimension;
    size_t *array_size;
    bool param_valid;
    size_t param_count;
    CcmmcSymbolType *param_list;
} CcmmcSymbolType;

typedef struct CcmmcSymbolAttr_struct {
    uint64_t addr;
} CcmmcSymbolAttr;

typedef struct CcmmcSymbol_struct CcmmcSymbol;
typedef struct CcmmcSymbol_struct {
    CcmmcSymbolKind kind;
    CcmmcSymbolType type;
    CcmmcSymbolAttr attr;
    const char *name;
    CcmmcSymbol *next;
} CcmmcSymbol;

typedef struct CcmmcSymbolScope_struct CcmmcSymbolScope;
typedef struct CcmmcSymbolScope_struct {
    CcmmcSymbol *hash_table[CCMMC_SYMBOL_SCOPE_HASH_TABLE_SIZE];
    CcmmcSymbolScope *all_next;
    CcmmcSymbolScope *current_next;
} CcmmcSymbolScope;

typedef struct CcmmcSymbolTable_struct {
    // ro
    CcmmcSymbolScope *all;
    CcmmcSymbolScope *all_last;
    // rw
    CcmmcSymbolScope *this_scope;
    CcmmcSymbolScope *current;
} CcmmcSymbolTable;

static inline bool ccmmc_symbol_type_is_scalar(CcmmcSymbolType type) {
    return type.array_dimension == 0 && !type.param_valid;
}
static inline bool ccmmc_symbol_type_is_array(CcmmcSymbolType type) {
    return type.array_dimension != 0;
}
static inline bool ccmmc_symbol_type_is_function(CcmmcSymbolType type) {
    return type.param_valid;
}
static inline bool ccmmc_symbol_is_scalar(CcmmcSymbol *symbol) {
    return ccmmc_symbol_type_is_scalar(symbol->type);
}
static inline bool ccmmc_symbol_is_array(CcmmcSymbol *symbol) {
    return ccmmc_symbol_type_is_array(symbol->type);
}
static inline bool ccmmc_symbol_is_function(CcmmcSymbol *symbol) {
    return ccmmc_symbol_type_is_function(symbol->type);
}

void             ccmmc_symbol_table_open_scope      (CcmmcSymbolTable *table);
void             ccmmc_symbol_table_reopen_scope    (CcmmcSymbolTable *table);
void             ccmmc_symbol_table_close_scope     (CcmmcSymbolTable *table);
void             ccmmc_symbol_table_init            (CcmmcSymbolTable *table);
void             ccmmc_symbol_table_insert          (CcmmcSymbolTable *table,
                                                     const char *name,
                                                     CcmmcSymbolKind kind,
                                                     CcmmcSymbolType type);
CcmmcSymbol     *ccmmc_symbol_table_retrieve        (CcmmcSymbolTable *table,
                                                     const char *name);
bool             ccmmc_symbol_scope_exist           (CcmmcSymbolScope *scope,
                                                     const char *name);


#endif
// vim: set sw=4 ts=4 sts=4 et:
