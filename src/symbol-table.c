#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "symbol-table.h"

#define TABLE_SIZE    256

static CcmmcSymbol *hash_table[TABLE_SIZE];

static int hash(char *str) {
    int idx = 0;
    while (*str) {
        idx = idx << 1;
        idx += *str;
        str++;
    }
    return (idx & (TABLE_SIZE-1));
}

/* returns the symbol table entry if found else NULL */
CcmmcSymbol *ccmmc_symbol_table_lookup(char *name) {
    int hash_key;
    CcmmcSymbol *symptr;
    if (!name)
        return NULL;
    hash_key = hash(name);
    symptr = hash_table[hash_key];

    while (symptr) {
        if (!(strcmp(name, symptr->lexeme)))
            return symptr;
        symptr = symptr->front;
    }
    return NULL;
}


void ccmmc_symbol_table_insert_id(char *name, int line_number) {
    int hash_key;
    CcmmcSymbol *ptr;
    CcmmcSymbol *symptr = malloc(sizeof(CcmmcSymbol));

    hash_key = hash(name);
    ptr = hash_table[hash_key];

    if (ptr == NULL) {
        /* first entry for this hash_key */
        hash_table[hash_key] = symptr;
        symptr->front = NULL;
        symptr->back = symptr;
    } else {
        symptr->front = ptr;
        ptr->back = symptr;
        symptr->back = symptr;
        hash_table[hash_key] = symptr;
    }

    strcpy(symptr->lexeme, name);
    symptr->line = line_number;
    symptr->counter = 1;
}

static void print_symbol(CcmmcSymbol *ptr) {
    printf(" Name = %s \n", ptr->lexeme);
    printf(" References = %d \n", ptr->counter);
}

void ccmmc_symbol_table_print(void) {
    puts("----- Symbol Table ---------");
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        CcmmcSymbol *symptr;
        symptr = hash_table[i];
        while (symptr != NULL)
        {
             printf("====>  index = %d\n", i);
             print_symbol(symptr);
             symptr = symptr->front;
        }
    }
}

// vim: set sw=4 ts=4 sts=4 et:
