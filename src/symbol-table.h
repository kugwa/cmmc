#ifndef CCMMC_HEADER_SYMBOL_TABLE_H
#define CCMMC_HEADER_SYMBOL_TABLE_H

typedef struct CcmmcSymbol_struct {
    char lexeme[256];
    struct CcmmcSymbol_struct *front;
    struct CcmmcSymbol_struct *back;
    int line;
    int counter;
} CcmmcSymbol;

CcmmcSymbol     *ccmmc_symbol_table_lookup          (char *name);
void             ccmmc_symbol_table_insert_id       (char *name,
                                                     int line_number);
void             ccmmc_symbol_table_print           (void);

#endif
// vim: set sw=4 ts=4 sts=4 et:
