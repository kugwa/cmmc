#ifndef CCMMC_HEADER_SYMBOL_TABLE_H
#define CCMMC_HEADER_SYMBOL_TABLE_H

struct symtab {
    char lexeme[256];
    struct symtab *front;
    struct symtab *back;
    int line;
    int counter;
};

typedef struct symtab symtab;
symtab* lookup(char *name);
void insertID(char *name, int line_number);
void printSymTab(void);
symtab **fillTab(int *len);

#endif
// vim: set sw=4 ts=4 sts=4 et:
