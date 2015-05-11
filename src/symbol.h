#ifndef _SYMBOL_H
#define _SYMBOL_H

#include <stdio.h>

struct node;
struct type;

#define FILE_SCOPE_SYMBOL_TABLE              0
#define FUNCTION_SCOPE_SYMBOL_TABLE          1
#define BLOCK_SCOPE_SYMBOL_TABLE             2

#define STACK_OFFSET_NOT_YET_DEFINED        -1

#define BEGINNING_STACK_OFFSET             100
struct symbol {
  char name[MAX_IDENTIFIER_LENGTH + 1];
  struct result result;
  struct symbol_table *owner_symbol_table;
  int stack_offset;
};

struct symbol_list {
  struct symbol symbol;
  struct symbol_list *next;
};

struct symbol_table {
  struct symbol_list *variables;
  int type_of_symbol_table;
  struct symbol_list *statement_labels;
  struct symbol_table *parent_symbol_table;
  int total_stack_offset;
};


void symbol_initialize_table(struct symbol_table *table, int type_of_symbol_table);
void symbol_add_from_translation_unit(struct symbol_table *table, struct node *translation_unit);
void symbol_add_from_statement_list(struct symbol_table *table, struct node *statement_list);
void symbol_print_table(FILE *output, struct symbol_table *table);
void calculate_stack_offsets_in_symbol_tables(struct symbol_table *table);

struct type *get_type_from_type_specifier(struct node *type_specifier);

extern FILE *error_output;
extern int symbol_table_num_errors;

#endif /* _SYMBOL_H */
