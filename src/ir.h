#ifndef _IR_H
#define _IR_H

#include <stdio.h>
#include <stdbool.h>

struct node;
struct symbol;
struct symbol_table;

#define OPERAND_NUMBER                  1
#define OPERAND_TEMPORARY               2
#define OPERAND_GENERATED_LABEL         3
#define OPERAND_IDENTIFIER              4
#define OPERAND_STRING                  5
#define OPERAND_NULL                    6

struct ir_operand {
  int kind;
  bool lvalue;

  union {
      unsigned long number;
      int temporary;
      struct {
          char identifier_name[MAX_IDENTIFIER_LENGTH+1];
          struct symbol *symbol;
      } identifier;
      int generated_label;
      struct {
          int generated_label;
          char name[MAX_STRING_LENGTH];
      } string_label;
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
#define IR_GENERATED_LABEL        10
#define IR_GOTO                   11
#define IR_FUNCTION_CALL          12
#define IR_ADDRESS_OF             13
#define IR_LOAD_WORD              14
#define IR_LESS_THAN              15
#define IR_LESS_THAN_OR_EQ_TO     16
#define IR_GREATER_THAN           17
#define IR_GREATER_THAN_OR_EQ_TO  18
#define IR_SHIFT_LEFT             19
#define IR_SHIFT_RIGHT            20
#define IR_EQUAL_TO               21
#define IR_NOT_EQUAL_TO           22
#define IR_BITWISE_OR             23
#define IR_BITWISE_XOR            24
#define IR_BITWISE_AND            25
#define IR_BIFEQZ                 26
#define IR_BIFNOTEQZ              27
#define IR_STORE_WORD             28
#define IR_SIZEOF                 29
#define IR_BITWISE_NOT            30
#define IR_LOGICAL_NOT            31
#define IR_NEGATION               32
#define IR_GOTO_IF_FALSE          33
#define IR_RETURN                 34
#define IR_FUNCTION_BEGIN         35
#define IR_FUNCTION_END           36
#define IR_GOTO_IF_TRUE           37
#define IR_CAST_TO_U_WORD         38
#define IR_CAST_TO_S_WORD         39
#define IR_CAST_TO_U_HALFWORD     40
#define IR_CAST_TO_S_HALFWORD     41
#define IR_CAST_TO_U_BYTE         42
#define IR_CAST_TO_S_BYTE         43
#define IR_FUNCTION_PARAMETER     44
#define IR_RESULTWORD             45
#define IR_CAST_WORD_TO_U_BYTE    46
#define IR_CAST_WORD_TO_S_BYTE    47
#define IR_CAST_HWORD_TO_U_BYTE   48
#define IR_CAST_HWORD_TO_S_BYTE   49
#define IR_CAST_HWORD_TO_U_WORD   50
#define IR_CAST_HWORD_TO_S_WORD   51
#define IR_CAST_BYTE_TO_U_WORD    52
#define IR_CAST_BYTE_TO_S_WORD    53
#define IR_CAST_WORD_TO_U_HWORD   54
#define IR_CAST_WORD_TO_S_HWORD   55
#define IR_CAST_BYTE_TO_U_HWORD   56
#define IR_CAST_BYTE_TO_S_HWORD   57
#define IR_LOAD_SIGNED_BYTE       58
#define IR_LOAD_SIGNED_HALFWORD   59
#define IR_STORE_SIGNED_BYTE      60
#define IR_STORE_SIGNED_HALFWORD  61


struct ir_instruction {
  int kind;
  struct ir_instruction *prev, *next;
  struct ir_operand operands[3];
};

struct ir_section {
  struct ir_instruction *first, *last;
};

void ir_generate_for_program(struct node *program);

void ir_print_section(FILE *output, struct ir_section *section);

void ir_print_section_reverse(FILE *output, struct ir_section *section);

struct ir_section *ir_section(struct ir_instruction *first, struct ir_instruction *last);

extern FILE *error_output;
extern int ir_generation_num_errors;
#endif
