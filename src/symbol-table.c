#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "common.h"
#include "symbol-table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int hash(const char *str)
{
    int idx = 0;
    for (; *str; str++) {
        idx = idx << 1;
        idx += *str;
    }
    return (idx & (CCMMC_SYMBOL_SCOPE_HASH_TABLE_SIZE - 1));
}

void ccmmc_symbol_table_init(CcmmcSymbolTable *table)
{
    table->all = NULL;
    table->all_last = NULL;
    table->current = NULL;
}

// push a scope on the stack
void ccmmc_symbol_table_open_scope(CcmmcSymbolTable *table)
{
    CcmmcSymbolScope *scope = malloc(sizeof(CcmmcSymbolScope));
    ERR_FATAL_CHECK(scope, malloc);
    for (int i = 0; i < CCMMC_SYMBOL_SCOPE_HASH_TABLE_SIZE; i++)
        scope->hash_table[i] = NULL;
    if (table->all == NULL) {
        table->all = scope;
        table->all_last = scope;
    } else {
        table->all_last->all_next = scope;
        table->all_last = scope;
    }
    scope->all_next = NULL;
    scope->current_next = table->current;
    table->current = scope;
}

// pop a scope from the stack
void ccmmc_symbol_table_close_scope(CcmmcSymbolTable *table)
{
    CcmmcSymbolScope *closing = table->current;
    table->current = closing->current_next;
}

void ccmmc_symbol_table_insert(CcmmcSymbolTable *table,
    const char *name, CcmmcSymbolKind kind, CcmmcSymbolType type)
{
    int key = hash(name);
    CcmmcSymbol *symbol = malloc(sizeof(CcmmcSymbol));
    ERR_FATAL_CHECK(symbol, malloc);
    symbol->kind = kind;
    symbol->type = type;
    symbol->attr.addr = 0;
    symbol->name = name;
    symbol->next = table->current->hash_table[key];
    table->current->hash_table[key] = symbol;
}

CcmmcSymbol *ccmmc_symbol_table_retrive (
    CcmmcSymbolTable *table, const char *name)
{
    if (name == NULL)
        return NULL;

    int key = hash(name);
    for (CcmmcSymbolScope *scope = table->current; scope; scope = scope->current_next) {
        for (CcmmcSymbol *symbol = scope->hash_table[key]; symbol; symbol = symbol->next) {
            if (strcmp(name, symbol->name) == 0)
                return symbol;
        }
    }
    return NULL;
}

bool ccmmc_symbol_scope_exist(CcmmcSymbolScope *scope, const char *name)
{
    int key = hash(name);
    for (CcmmcSymbol *symbol = scope->hash_table[key];
         symbol != NULL; symbol = symbol->next) {
        if (strcmp(name, symbol->name) == 0)
            return true;
    }
    return false;
}

// vim: set sw=4 ts=4 sts=4 et:
