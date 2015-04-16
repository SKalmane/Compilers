#ifndef _IR_H
#define _IR_H

#include <stdio.h>
#include <stdbool.h>

struct node;
struct symbol;
struct symbol_table;

#define OPERAND_NUMBER                  1
#define OPERAND_TEMPORARY               2
#define OPERAND_BRANCH_LABEL            3
#define OPERAND_IDENTIFIER              4
struct ir_operand {
  int kind;
  bool lvalue;
    
  union {
    unsigned long number;
    int temporary;
    char identifier_name[MAX_IDENTIFIER_LENGTH];
  } data;
};

#define IR_NO_OPERATION            1
#define IR_MULTIPLY                2
#define IR_DIVIDE                  3
#define IR_ADD                     4
#define IR_SUBTRACT                5
#define IR_REMAINDER               6
#define IR_LOAD_IMMEDIATE          7
#define IR_COPY                    8
#define IR_PRINT_NUMBER            9
#define IR_GENERATE_LABEL         10
#define IR_GOTO                   11
#define IR_FUNCTION_CALL          12
#define IR_ADDRESS_OF             13
#define IR_LOAD_WORD              14

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
