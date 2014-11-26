#ifndef _IR_H
#define _IR_H

#include <stdio.h>
#include <stdbool.h>

struct node;
struct symbol;
struct symbol_table;

#define OPERAND_NUMBER     1
#define OPERAND_TEMPORARY  2
struct ir_operand {
  int kind;

  union {
    unsigned long number;
    int temporary;
  } data;
};

#define IR_MULTIPLY                1
#define IR_DIVIDE                  2
#define IR_ADD                     3
#define IR_SUBTRACT                4
#define IR_LOAD_IMMEDIATE          5
#define IR_COPY                    6
#define IR_PRINT_NUMBER            7
struct ir_instruction {
  int kind;
  struct ir_instruction *prev, *next;
  struct ir_operand operands[3];
};

struct ir_section {
  struct ir_instruction *first, *last;
};

void ir_generate_for_statement_list(struct node *statement_list);

void ir_print_section(FILE *output, struct ir_section *section);


extern FILE *error_output;
extern int ir_generation_num_errors;
#endif
