#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "node.h"
#include "symbol.h"
#include "type.h"
#include "ir.h"

int ir_generation_num_errors;

int next_temporary;

/* xxx: Things to do:
 * Casting
 * Subscript Exprs
 * Signed/Unsigned additions
 * Break/continue statements
 * Compound assignments - should not evaluate twice
 */

/************************
 * CREATE IR STRUCTURES *
 ************************/

/*
 * An IR section is just a list of IR instructions. Each node has an associated
 * IR section if any code is required to implement it.
 */
struct ir_section *ir_section(struct ir_instruction *first, struct ir_instruction *last) {
  struct ir_section *code;
  code = malloc(sizeof(struct ir_section));
  assert(NULL != code);

  code->first = first;
  code->last = last;
  return code;
}

struct ir_section *ir_copy(struct ir_section *orig) {
  return ir_section(orig->first, orig->last);
}

/*
 * This joins two IR sections together into a new IR section.
 */
struct ir_section *ir_concatenate(struct ir_section *before, struct ir_section *after) {
  /* patch the two sections together */
  before->last->next = after->first;
  after->first->prev = before->last;

  return ir_section(before->first, after->last);
}

static struct ir_section *ir_append(struct ir_section *section,
                                                           struct ir_instruction *instruction) {
  if (NULL == section) {
    section = ir_section(instruction, instruction);

  } else if (NULL == section->first || NULL == section->last) {
    assert(NULL == section->first && NULL == section->last);
    section->first = instruction;
    section->last = instruction;
    instruction->prev = NULL;
    instruction->next = NULL;

  } else {
    instruction->next = section->last->next;
    if (NULL != instruction->next) {
      instruction->next->prev = instruction;
    }
    section->last->next = instruction;

    instruction->prev = section->last;
    section->last = instruction;
  }
  return section;
}

/*
 * An IR instruction represents a single 3-address statement.
 */
struct ir_instruction *ir_instruction(int kind) {
  struct ir_instruction *instruction;

  instruction = malloc(sizeof(struct ir_instruction));
  assert(NULL != instruction);

  instruction->kind = kind;

  instruction->next = NULL;
  instruction->prev = NULL;

  return instruction;
}

static void ir_operand_number(struct ir_instruction *instruction, int position, struct node *number) {
  instruction->operands[position].kind = OPERAND_NUMBER;
  instruction->operands[position].data.number = number->data.number.value;
}

static void ir_operand_identifier(struct ir_instruction *instruction, int position, struct node *identifier) {
  instruction->operands[position].kind = OPERAND_IDENTIFIER;
  strncpy(instruction->operands[position].data.identifier_name, identifier->data.identifier.name,
          MAX_IDENTIFIER_LENGTH);
}

static void ir_operand_temporary(struct ir_instruction *instruction, int position) {
  instruction->operands[position].kind = OPERAND_TEMPORARY;
  instruction->operands[position].data.temporary = next_temporary++;
}

static void ir_generate_label(struct ir_instruction *instruction) {
  static int next_generated_label;
  instruction->operands[0].kind = OPERAND_GENERATED_LABEL;
  instruction->operands[0].data.generated_label = next_generated_label++;
}

static void ir_generate_string_label(struct ir_instruction **instruction,
                                     struct node *string) {
  static int next_generated_string_label;
  ir_operand_temporary((*instruction), 0);
  (*instruction)->operands[1].kind = OPERAND_STRING;
  (*instruction)->operands[1].data.string_label.generated_label = next_generated_string_label++;
  strncpy((*instruction)->operands[1].data.string_label.name, string->data.string.name,
      MAX_STRING_LENGTH);
}

/* static void ir_operand_generated_label(struct ir_instruction *instruction, int position) { */
/*     static int next_label; */
/*     instruction->operands[position].kind = OPERAND_BRANCH_LABEL; */
/*     instruction->operands[position].data.temporary = next_label++; */
/* } */

static void ir_operand_copy(struct ir_instruction *instruction, int position, struct ir_operand *operand) {
  instruction->operands[position] = *operand;
}

static void ir_generate_gotoFalseOrTrue(struct ir_instruction *instruction, struct ir_operand *ir_operand,
                                    struct ir_instruction *label_instruction) {
    ir_operand_copy(instruction, 1, ir_operand);
    ir_operand_copy(instruction, 0, &label_instruction->operands[0]);
}

static void ir_generate_goto(struct ir_instruction *instruction, struct ir_instruction *label_instruction) {
    ir_operand_copy(instruction, 0, &label_instruction->operands[0]);
}

/*******************************
 * GENERATE IR FOR EXPRESSIONS *
 *******************************/
void ir_generate_for_number(struct node *number) {
  struct ir_instruction *instruction;
  assert(NODE_NUMBER == number->kind);

  instruction = ir_instruction(IR_LOAD_IMMEDIATE);
  ir_operand_temporary(instruction, 0);
  ir_operand_number(instruction, 1, number);

  number->ir = ir_section(instruction, instruction);

  number->data.number.result.ir_operand = &instruction->operands[0];
  number->data.number.result.ir_operand->lvalue = false;
}

void ir_generate_for_identifier(struct node *identifier) {
  struct ir_instruction *instruction;
  assert(NODE_IDENTIFIER == identifier->kind);
  /* load the address as an lvalue */
  instruction = ir_instruction(IR_ADDRESS_OF);
  ir_operand_temporary(instruction, 0);
  ir_operand_identifier(instruction, 1, identifier);

  identifier->ir = ir_section(instruction, instruction);
  identifier->data.identifier.symbol->result.ir_operand = &instruction->operands[0];
  identifier->data.identifier.symbol->result.ir_operand->lvalue = true;
  assert(NULL != identifier->data.identifier.symbol->result.ir_operand);
}

void ir_generate_for_string(struct node *string) {
  struct ir_instruction *instruction;
  assert(NODE_STRING == string->kind);
  /* load the address as an lvalue */
  instruction = ir_instruction(IR_ADDRESS_OF);
  ir_generate_string_label(&instruction, string);

  string->ir = ir_section(instruction, instruction);

  string->data.string.result.ir_operand = &instruction->operands[0];
  string->data.string.result.ir_operand->lvalue = false;

  assert(NULL != string->data.string.result.ir_operand);
}

void ir_generate_for_expression(struct node *expression,
                                struct ir_instruction *function_end_label,
                                struct ir_instruction *inner_loop_end_label);

void ir_generate_for_conversion_to_rvalue(struct node *lvalue_node) {
    struct ir_instruction *instruction;

    /* Do nothing if the node is already an rvalue */
    if(!node_get_result(lvalue_node)->ir_operand->lvalue) {
        return;
    }

    instruction = ir_instruction(IR_LOAD_WORD);
    ir_operand_temporary(instruction, 0);
    ir_operand_copy(instruction, 1,
                    node_get_result(lvalue_node)->ir_operand);
    lvalue_node->ir = ir_append(lvalue_node->ir, instruction);
    node_get_result(lvalue_node)->ir_operand = &instruction->operands[0];
    node_get_result(lvalue_node)->ir_operand->lvalue = false;
}

void ir_generate_for_arithmetic_binary_operation(int kind, struct node *binary_operation) {
    struct ir_instruction *instruction;
  assert(NODE_BINARY_OPERATION == binary_operation->kind);

  ir_generate_for_expression(binary_operation->data.binary_operation.left_operand, NULL, NULL);
  ir_generate_for_expression(binary_operation->data.binary_operation.right_operand, NULL, NULL);

  instruction = ir_instruction(kind);
  ir_operand_temporary(instruction, 0);
  /* binary operations need both operands to be rvalues. */

  if(node_get_result(binary_operation->data.binary_operation.left_operand)->ir_operand->lvalue) {
      ir_generate_for_conversion_to_rvalue(binary_operation->data.binary_operation.left_operand);
  }

  if(node_get_result(binary_operation->data.binary_operation.right_operand)->ir_operand->lvalue) {
      ir_generate_for_conversion_to_rvalue(binary_operation->data.binary_operation.right_operand);
  }

  ir_operand_copy(instruction, 1,
                  node_get_result(binary_operation->data.binary_operation.left_operand)->ir_operand);
  ir_operand_copy(instruction, 2,
                  node_get_result(binary_operation->data.binary_operation.right_operand)->ir_operand);
  binary_operation->ir = ir_concatenate(binary_operation->data.binary_operation.left_operand->ir,
                                        binary_operation->data.binary_operation.right_operand->ir);
  ir_append(binary_operation->ir, instruction);
  binary_operation->data.binary_operation.result.ir_operand = &instruction->operands[0];
  binary_operation->data.binary_operation.result.ir_operand->lvalue = false;
}

void ir_generate_for_simple_assignment(struct node *binary_operation) {
  struct ir_instruction *instruction;
  struct node *left;

  assert(NODE_BINARY_OPERATION == binary_operation->kind);

  left = binary_operation->data.binary_operation.left_operand;
  ir_generate_for_expression(left, NULL, NULL);
  assert(NULL != node_get_result(left)->ir_operand);

  if(!node_get_result(left)->ir_operand->lvalue) {
      ir_generation_num_errors++;
      printf("ERROR: The left hand side of assignment operation is not an lvalue\n");
  }

  ir_generate_for_expression(binary_operation->data.binary_operation.right_operand, NULL, NULL);

  if(node_get_result(binary_operation->data.binary_operation.right_operand)->ir_operand->lvalue) {
      ir_generate_for_conversion_to_rvalue(binary_operation->data.binary_operation.right_operand);
  }

  instruction = ir_instruction(IR_COPY);

  ir_operand_copy(instruction, 0,
                  node_get_result(left)->ir_operand);

  ir_operand_copy(instruction, 1,
                  node_get_result(binary_operation->data.binary_operation.right_operand)->ir_operand);

  binary_operation->ir = ir_concatenate(left->ir,
                                        binary_operation->data.binary_operation.right_operand->ir);
  ir_append(binary_operation->ir, instruction);

  binary_operation->data.binary_operation.result.ir_operand = &instruction->operands[0];
  binary_operation->data.binary_operation.result.ir_operand->lvalue = true;
}

void ir_generate_for_binary_operation(struct node *binary_operation);

void ir_generate_for_compound_assignment(int kind, struct node *binary_operation) {
  struct ir_instruction *instruction;
  struct node *left;
  assert(NODE_BINARY_OPERATION == binary_operation->kind);

  left = binary_operation->data.binary_operation.left_operand;

  binary_operation->data.binary_operation.operation = kind;
  ir_generate_for_binary_operation(binary_operation);

  ir_generate_for_expression(left, NULL, NULL);
  assert(NODE_IDENTIFIER == left->kind);
  assert(NULL != node_get_result(left)->ir_operand);
  if(!node_get_result(left)->ir_operand->lvalue) {
      ir_generation_num_errors++;
      printf("ERROR: The left hand side of assignment operation is not an lvalue\n");
  }
  binary_operation->ir = ir_concatenate(binary_operation->ir, left->ir);

  instruction = ir_instruction(IR_COPY);

  ir_operand_copy(instruction, 0,
                  node_get_result(left)->ir_operand);

  ir_operand_copy(instruction, 1,
                  node_get_result(binary_operation)->ir_operand);

  ir_append(binary_operation->ir, instruction);

  binary_operation->data.binary_operation.result.ir_operand = &instruction->operands[0];
  binary_operation->data.binary_operation.result.ir_operand->lvalue = true;
}

void ir_generate_for_ternary_operation(struct node *ternary_operation) {
    struct ir_instruction *storeword_instruction1, *storeword_instruction2;
    struct ir_instruction *label_instruction1, *label_instruction2;
    struct ir_instruction *gotoIf_instruction1, *goto_instruction;

    struct node *first_operand = ternary_operation->data.ternary_operation.first_operand;
    struct node *second_operand = ternary_operation->data.ternary_operation.second_operand;
    struct node *third_operand = ternary_operation->data.ternary_operation.third_operand;

    assert(NODE_TERNARY_OPERATION == ternary_operation->kind);

    ir_generate_for_expression(first_operand, NULL, NULL);
    if(node_get_result(first_operand)->ir_operand->lvalue) {
        ir_generate_for_conversion_to_rvalue(first_operand);
    }

    label_instruction1 = ir_instruction(IR_GENERATED_LABEL);
    ir_generate_label(label_instruction1);

    label_instruction2 = ir_instruction(IR_GENERATED_LABEL);
    ir_generate_label(label_instruction2);

    gotoIf_instruction1 = ir_instruction(IR_GOTO_IF_FALSE);
    ir_generate_gotoFalseOrTrue(gotoIf_instruction1, node_get_result(first_operand)->ir_operand, label_instruction1);

    ir_generate_for_expression(second_operand, NULL, NULL);
    if(node_get_result(second_operand)->ir_operand->lvalue) {
        ir_generate_for_conversion_to_rvalue(second_operand);
    }

    storeword_instruction1 = ir_instruction(IR_STORE_WORD);
    ir_operand_temporary(storeword_instruction1, 0);
    ir_operand_copy(storeword_instruction1, 1,
                    node_get_result(second_operand)->ir_operand);

    goto_instruction = ir_instruction(IR_GOTO);
    ir_generate_goto(goto_instruction, label_instruction2);

    ir_generate_for_expression(third_operand, NULL, NULL);
    if(node_get_result(third_operand)->ir_operand->lvalue) {
        ir_generate_for_conversion_to_rvalue(third_operand);
    }

    storeword_instruction2 = ir_instruction(IR_STORE_WORD);
    ir_operand_copy(storeword_instruction2, 0,
                    &storeword_instruction1->operands[0]);
    ir_operand_copy(storeword_instruction2, 1,
                    node_get_result(third_operand)->ir_operand);

    /* Create the IR by adding up all the instructions */
    ternary_operation->ir = ir_copy(first_operand->ir);
    ir_append(ternary_operation->ir, gotoIf_instruction1);
    ternary_operation->ir = ir_concatenate(ternary_operation->ir, second_operand->ir);
    ir_append(ternary_operation->ir, storeword_instruction1);
    ir_append(ternary_operation->ir, goto_instruction);
    ir_append(ternary_operation->ir, label_instruction1);
    ternary_operation->ir = ir_concatenate(ternary_operation->ir, third_operand->ir);
    ir_append(ternary_operation->ir, storeword_instruction2);
    ir_append(ternary_operation->ir, label_instruction2);

    ternary_operation->data.ternary_operation.result.ir_operand = &storeword_instruction2->operands[0];
    ternary_operation->data.ternary_operation.result.ir_operand->lvalue = false;
}

void ir_generate_for_logical_binary_operation(int kind, struct node *binary_operation) {
    struct ir_instruction *storeword_instruction1, *storeword_instruction2;
    struct ir_instruction *label_instruction1, *label_instruction2;
    struct ir_instruction *gotoIf_instruction1, *gotoIf_instruction2, *goto_instruction;
    struct ir_instruction *constant_instruction1, *constant_instruction2;

    struct node *left = binary_operation->data.binary_operation.left_operand;
    struct node *right = binary_operation->data.binary_operation.right_operand;

    assert(NODE_BINARY_OPERATION == binary_operation->kind);
    printf("Kind is %d\n", kind);

    assert((kind == BINOP_LOGICAL_OR_EXPR) || (kind == BINOP_LOGICAL_AND_EXPR));

    ir_generate_for_expression(left, NULL, NULL);
    if(node_get_result(left)->ir_operand->lvalue) {
        ir_generate_for_conversion_to_rvalue(left);
    }

    label_instruction1 = ir_instruction(IR_GENERATED_LABEL);
    ir_generate_label(label_instruction1);

    label_instruction2 = ir_instruction(IR_GENERATED_LABEL);
    ir_generate_label(label_instruction2);

    ir_generate_for_expression(right, NULL, NULL);
    if(node_get_result(right)->ir_operand->lvalue) {
        ir_generate_for_conversion_to_rvalue(right);
    }

    constant_instruction1 = ir_instruction(IR_LOAD_IMMEDIATE);
    constant_instruction2 = ir_instruction(IR_LOAD_IMMEDIATE);

    switch (kind) {
      case BINOP_LOGICAL_OR_EXPR:
        /* For the OR Expr, we need to branch if the first
         * expr is true. Otherwise, we need to evaluate the
         * second expr
         */
        gotoIf_instruction1 = ir_instruction(IR_GOTO_IF_TRUE);

        gotoIf_instruction2 = ir_instruction(IR_GOTO_IF_TRUE);

        /* Create the constInt instruction for the number 0 */
        ir_operand_temporary(constant_instruction1, 0);
        constant_instruction1->operands[1].kind = OPERAND_NUMBER;
        constant_instruction1->operands[1].data.number = 0;
        constant_instruction1->operands[1].lvalue = false;

        /* Create the constInt instruction for the number 1 */
        ir_operand_temporary(constant_instruction2, 0);
        constant_instruction2->operands[1].kind = OPERAND_NUMBER;
        constant_instruction2->operands[1].data.number = 1;
        constant_instruction2->operands[1].lvalue = false;

        break;
      case BINOP_LOGICAL_AND_EXPR:
        /* For the AND Expr, we need to branch if the first
         * expr is false. Otherwise, we need to evaluate the
         * second expr
         */
        gotoIf_instruction1 = ir_instruction(IR_GOTO_IF_FALSE);

        gotoIf_instruction2 = ir_instruction(IR_GOTO_IF_FALSE);

        /* Create the constInt instruction for the number 0 */
        ir_operand_temporary(constant_instruction1, 0);
        constant_instruction1->operands[1].kind = OPERAND_NUMBER;
        constant_instruction1->operands[1].data.number = 1;
        constant_instruction1->operands[1].lvalue = false;

        /* Create the constInt instruction for the number 1 */
        ir_operand_temporary(constant_instruction2, 0);
        constant_instruction2->operands[1].kind = OPERAND_NUMBER;
        constant_instruction2->operands[1].data.number = 0;
        constant_instruction2->operands[1].lvalue = false;
        break;
      default:
        assert(0);
        break;
    }

    /* We have prepared the branching instructions and the constInts based upon
     * whether the operation is && or ||.  Now crate the instructions completely
     * by using the labels etc. This is the same irrespective of && or ||
     */
    ir_generate_gotoFalseOrTrue(gotoIf_instruction1, node_get_result(left)->ir_operand, label_instruction1);

    ir_generate_gotoFalseOrTrue(gotoIf_instruction2, node_get_result(right)->ir_operand, label_instruction1);

    storeword_instruction1 = ir_instruction(IR_STORE_WORD);
    ir_operand_temporary(storeword_instruction1, 0);
    ir_operand_copy(storeword_instruction1, 1,
                    &constant_instruction1->operands[0]);

    goto_instruction = ir_instruction(IR_GOTO);
    ir_generate_goto(goto_instruction, label_instruction2);

    storeword_instruction2 = ir_instruction(IR_STORE_WORD);
    ir_operand_copy(storeword_instruction2, 0,
                    &storeword_instruction1->operands[0]);
    ir_operand_copy(storeword_instruction2, 1,
                    &constant_instruction2->operands[0]);

    /* Create the IR by adding up all the instructions */
    binary_operation->ir = ir_copy(left->ir);
    ir_append(binary_operation->ir, gotoIf_instruction1);
    binary_operation->ir = ir_concatenate(binary_operation->ir, right->ir);
    ir_append(binary_operation->ir, gotoIf_instruction2);
    ir_append(binary_operation->ir, constant_instruction1);
    ir_append(binary_operation->ir, storeword_instruction1);
    ir_append(binary_operation->ir, goto_instruction);
    ir_append(binary_operation->ir, label_instruction1);
    ir_append(binary_operation->ir, constant_instruction2);
    ir_append(binary_operation->ir, storeword_instruction2);
    ir_append(binary_operation->ir, label_instruction2);

    binary_operation->data.binary_operation.result.ir_operand = &storeword_instruction2->operands[0];
    binary_operation->data.binary_operation.result.ir_operand->lvalue = false;
}

void ir_generate_for_increment_decrement_operation(int kind, int is_prefix,
                                                   struct node *the_operand) {
    struct ir_instruction *add_instruction = ir_instruction(kind);
    struct ir_instruction *constant_inc_instruction = ir_instruction(IR_LOAD_IMMEDIATE);
    struct ir_instruction *store_instruction = ir_instruction(IR_STORE_WORD);
    struct ir_operand *addressOfOperand;
    struct ir_operand *valueOfOperandBeforeIncrementing;

    ir_generate_for_expression(the_operand, NULL, NULL);
    if(!node_get_result(the_operand)->ir_operand->lvalue) {
        ir_generation_num_errors++;
        printf("ERROR: The operand to the unary operation must be a modifiable lvalue");
    }

    addressOfOperand = node_get_result(the_operand)->ir_operand;

    /* First convert the operand to an rvalue */
    ir_generate_for_conversion_to_rvalue(the_operand);

    valueOfOperandBeforeIncrementing = node_get_result(the_operand)->ir_operand;

    /* Create the constInt instruction for the number 1 */
    ir_operand_temporary(constant_inc_instruction, 0);
    constant_inc_instruction->operands[1].kind = OPERAND_NUMBER;
    constant_inc_instruction->operands[1].data.number = 1;
    constant_inc_instruction->operands[1].lvalue = false;

    /* Add/Subtract 1 to the operand */
    ir_operand_temporary(add_instruction, 0);
    ir_operand_copy(add_instruction, 1,
                  node_get_result(the_operand)->ir_operand);
    ir_operand_copy(add_instruction, 2,
                    &constant_inc_instruction->operands[0]);

    /*Store the value back into the operand */
    ir_operand_copy(store_instruction, 0, addressOfOperand);
    ir_operand_copy(store_instruction, 1, &add_instruction->operands[0]);

    ir_append(the_operand->ir, constant_inc_instruction);
    ir_append(the_operand->ir, add_instruction);
    ir_append(the_operand->ir, store_instruction);

    if(is_prefix) {
        node_get_result(the_operand)->ir_operand = &add_instruction->operands[0];
    } else {
        node_get_result(the_operand)->ir_operand = valueOfOperandBeforeIncrementing;
    }
    node_get_result(the_operand)->ir_operand->lvalue = false;
}

void ir_generate_for_simple_unary_operation(int kind, struct node *the_operand) {
    struct ir_instruction *instruction;
    ir_generate_for_expression(the_operand, NULL, NULL);
    ir_generate_for_conversion_to_rvalue(the_operand);
    instruction = ir_instruction(kind);
    ir_operand_temporary(instruction, 0);
    ir_operand_copy(instruction, 1, node_get_result(the_operand)->ir_operand);
    ir_append(the_operand->ir, instruction);
    node_get_result(the_operand)->ir_operand = &instruction->operands[0];
    node_get_result(the_operand)->ir_operand->lvalue = false;
}

void ir_generate_for_unary_operation(struct node *unary_operation) {
    struct node *the_operand = unary_operation->data.unary_operation.the_operand;
    /* struct ir_instruction *instruction; */
    assert(NODE_UNARY_OPERATION == unary_operation->kind);

    switch(unary_operation->data.unary_operation.operation) {
      case UNARYOP_PREFIX_INCREMENT:
        ir_generate_for_increment_decrement_operation(IR_ADD, true, the_operand);
        break;
      case UNARYOP_PREFIX_DECREMENT:
        ir_generate_for_increment_decrement_operation(IR_SUBTRACT, true, the_operand);
        break;
      case UNARYOP_POSTFIX_INCREMENT:
        ir_generate_for_increment_decrement_operation(IR_ADD, false, the_operand);
        break;
      case UNARYOP_POSTFIX_DECREMENT:
        ir_generate_for_increment_decrement_operation(IR_SUBTRACT, false, the_operand);
        break;
      case UNARYOP_SIZEOF:
        ir_generate_for_simple_unary_operation(IR_SIZEOF, the_operand);
        break;
      case UNARYOP_BITWISE_NOT:
        ir_generate_for_simple_unary_operation(IR_BITWISE_NOT, the_operand);
        break;
      case UNARYOP_LOGICAL_NOT:
        ir_generate_for_simple_unary_operation(IR_LOGICAL_NOT, the_operand);
        break;
      case UNARYOP_NEGATION:
        ir_generate_for_simple_unary_operation(IR_NEGATION, the_operand);
        break;
      case UNARYOP_PLUS:
        /* nothing to do. everything will remain the same */
        ir_generate_for_expression(the_operand, NULL, NULL);
        ir_generate_for_conversion_to_rvalue(the_operand);
        break;
      case UNARYOP_ADDRESS_OF:
        ir_generate_for_expression(the_operand, NULL, NULL);
        if(!node_get_result(the_operand)->ir_operand->lvalue) {
            ir_generation_num_errors++;
            printf("ERROR: The operand to a unary operation must be an lvalue\n");
        }
        node_get_result(the_operand)->ir_operand->lvalue = false;
        break;
      case UNARYOP_INDIRECTION:
        ir_generate_for_expression(the_operand, NULL, NULL);
        if(node_get_result(the_operand)->ir_operand->lvalue) {
            ir_generation_num_errors++;
            printf("ERROR: The operand to the indirection operation must be an rvalue\n");
        }
        ir_generate_for_conversion_to_rvalue(the_operand);
        node_get_result(the_operand)->ir_operand->lvalue = true;
        break;
      default:
        printf("Kind of unary operation: %d\n", unary_operation->data.unary_operation.operation);
        assert(0);
        break;
    }
    unary_operation->ir = the_operand->ir;
    node_get_result(unary_operation)->ir_operand = node_get_result(the_operand)->ir_operand;
}

void ir_generate_for_unary_casting_expr(struct ir_operand *ir_operand, 
                                        struct node *unary_casting_expr) {
    struct node *the_operand = unary_casting_expr->data.unary_operation.the_operand;
    struct ir_instruction *instruction;
    if(the_operand->kind == NODE_TYPE_SPECIFIER) {
        switch(the_operand->data.type_specifier.kind_of_type_specifier) {
          case UNSIGNED_CHARACTER_TYPE:
            instruction = ir_instruction(IR_CAST_TO_U_BYTE);
            break;
          case SIGNED_CHARACTER_TYPE:
            instruction = ir_instruction(IR_CAST_TO_S_BYTE);
            break;
          case UNSIGNED_SHORT_INT:
            instruction = ir_instruction(IR_CAST_TO_U_HALFWORD);
            break;
          case SIGNED_SHORT_INT:
            instruction = ir_instruction(IR_CAST_TO_S_HALFWORD);
            break;
          case UNSIGNED_LONG_INT:
          case UNSIGNED_INT:
            instruction = ir_instruction(IR_CAST_TO_U_WORD);
            break;
          case SIGNED_LONG_INT:
          case SIGNED_INT:
            instruction = ir_instruction(IR_CAST_TO_S_WORD);
            break;
          case VOID_TYPE:
            ir_generation_num_errors++;
            printf("ERROR: Casting to a void type that is not a pointer is not allowed\n");
            instruction = ir_instruction(IR_NO_OPERATION);
            break;
          default:
            assert(0);
            break;
        }
    } else {
        /* There is an abstract declarator involved, it is a pointer type */
        instruction = ir_instruction(IR_CAST_TO_U_WORD);
    }
    ir_operand_temporary(instruction, 0);
    ir_operand_copy(instruction, 1, ir_operand);
    ir_append(the_operand->ir, instruction);
    unary_casting_expr->ir = ir_section(instruction, instruction);
    node_get_result(unary_casting_expr)->ir_operand = &instruction->operands[0];
    node_get_result(unary_casting_expr)->ir_operand->lvalue = false;
}

void ir_generate_for_cast_expr(struct node *cast_expr) {
    struct node *unary_casting_expr = cast_expr->data.cast_expr.unary_casting_expr;
    struct node *cast_expr_within = cast_expr->data.cast_expr.cast_expr;

    ir_generate_for_expression(cast_expr_within, NULL, NULL);

    ir_generate_for_unary_casting_expr(node_get_result(cast_expr_within)->ir_operand, 
                                       unary_casting_expr);
    cast_expr->ir = ir_concatenate(cast_expr_within->ir, unary_casting_expr->ir);
    node_get_result(cast_expr)->ir_operand = node_get_result(unary_casting_expr)->ir_operand;
    node_get_result(cast_expr)->ir_operand->lvalue = false;
}

void ir_generate_for_binary_operation(struct node *binary_operation) {
  assert(NODE_BINARY_OPERATION == binary_operation->kind);

  switch (binary_operation->data.binary_operation.operation) {
    case BINOP_MULTIPLICATION:
      ir_generate_for_arithmetic_binary_operation(IR_MULTIPLY, binary_operation);
      break;

    case BINOP_DIVISION:
      ir_generate_for_arithmetic_binary_operation(IR_DIVIDE, binary_operation);
      break;

    case BINOP_ADDITION:
      ir_generate_for_arithmetic_binary_operation(IR_ADD, binary_operation);
      break;

    case BINOP_SUBTRACTION:
      ir_generate_for_arithmetic_binary_operation(IR_SUBTRACT, binary_operation);
      break;

    case BINOP_REMAINDER:
      ir_generate_for_arithmetic_binary_operation(IR_REMAINDER, binary_operation);
      break;

    case BINOP_ASSIGN:
      ir_generate_for_simple_assignment(binary_operation);
      break;

    case BINOP_ASSIGN_PLUS_EQUAL:
      ir_generate_for_compound_assignment(BINOP_ADDITION, binary_operation);
      break;

    case BINOP_ASSIGN_MINUS_EQUAL:
      ir_generate_for_compound_assignment(BINOP_SUBTRACTION, binary_operation);
      break;

    case BINOP_ASSIGN_ASTERISK_EQUAL:
      ir_generate_for_compound_assignment(BINOP_MULTIPLICATION, binary_operation);
      break;

    case BINOP_ASSIGN_SLASH_EQUAL:
      ir_generate_for_compound_assignment(BINOP_DIVISION, binary_operation);
      break;

    case BINOP_ASSIGN_PERCENT_EQUAL:
      ir_generate_for_compound_assignment(BINOP_REMAINDER, binary_operation);
      break;

    case BINOP_ASSIGN_LESS_LESS_EQUAL:
      ir_generate_for_compound_assignment(BINOP_SHIFT_LEFT, binary_operation);
      break;

    case BINOP_ASSIGN_GREATER_GREATER_EQUAL:
      ir_generate_for_compound_assignment(BINOP_SHIFT_RIGHT, binary_operation);
      break;

    case BINOP_ASSIGN_AMPERSAND_EQUAL:
      ir_generate_for_compound_assignment(BINOP_BITWISE_AND_EXPR, binary_operation);
      break;

    case BINOP_ASSIGN_CARET_EQUAL:
      ir_generate_for_compound_assignment(BINOP_BITWISE_XOR_EXPR, binary_operation);
      break;

    case BINOP_ASSIGN_VBAR_EQUAL:
      ir_generate_for_compound_assignment(BINOP_BITWISE_OR_EXPR, binary_operation);
      break;

    case BINOP_LESS_THAN:
        ir_generate_for_arithmetic_binary_operation(IR_LESS_THAN, binary_operation);
        break;

    case BINOP_LESS_THAN_OR_EQUAL_TO:
        ir_generate_for_arithmetic_binary_operation(IR_LESS_THAN_OR_EQ_TO, binary_operation);
        break;

    case BINOP_GREATER_THAN:
        ir_generate_for_arithmetic_binary_operation(IR_GREATER_THAN, binary_operation);
        break;

    case BINOP_GREATER_THAN_OR_EQUAL_TO:
        ir_generate_for_arithmetic_binary_operation(IR_GREATER_THAN_OR_EQ_TO, binary_operation);
        break;

    case BINOP_SHIFT_LEFT:
        ir_generate_for_arithmetic_binary_operation(IR_SHIFT_LEFT, binary_operation);
        break;

    case BINOP_SHIFT_RIGHT:
        ir_generate_for_arithmetic_binary_operation(IR_SHIFT_RIGHT, binary_operation);
        break;

    case BINOP_IS_EQUAL_TO:
        ir_generate_for_arithmetic_binary_operation(IR_EQUAL_TO, binary_operation);
        break;

    case BINOP_NOT_EQUAL_TO:
        ir_generate_for_arithmetic_binary_operation(IR_NOT_EQUAL_TO, binary_operation);
        break;

    case BINOP_BITWISE_OR_EXPR:
        ir_generate_for_arithmetic_binary_operation(IR_BITWISE_OR, binary_operation);
        break;

    case BINOP_BITWISE_XOR_EXPR:
        ir_generate_for_arithmetic_binary_operation(IR_BITWISE_XOR, binary_operation);
        break;

    case BINOP_BITWISE_AND_EXPR:
        ir_generate_for_arithmetic_binary_operation(IR_BITWISE_AND, binary_operation);
        break;

    case BINOP_LOGICAL_OR_EXPR:
    case BINOP_LOGICAL_AND_EXPR:
      ir_generate_for_logical_binary_operation(binary_operation->data.binary_operation.operation,
                                               binary_operation);
      break;

    default:
      assert(0);
      break;
  }
}

void ir_generate_for_compound_statement(struct node *compound_statement,
                                          struct ir_instruction *function_end_label,
                                          struct ir_instruction *inner_loop_end_label) {
  struct node *declaration_or_statement_list =
      compound_statement->data.compound_statement.declaration_or_statement_list;

  assert(NODE_COMPOUND_STATEMENT == compound_statement->kind);
  if(declaration_or_statement_list != NULL) {
      ir_generate_for_expression(declaration_or_statement_list,
                                 function_end_label, inner_loop_end_label);
      compound_statement->ir = declaration_or_statement_list->ir;
  }
}

struct node * get_function_name_from_children(struct node *child_node) {
    struct node *declarator, *identifier;
    if(child_node->kind == NODE_FUNCTION_DEF_SPECIFIER) {
        declarator = child_node->data.function_def_specifier.declarator;
    } else if(child_node->kind == NODE_POINTER_DECLARATOR){
        declarator = child_node->data.pointer_declarator.declarator;
    } else {
        assert(child_node->kind == NODE_FUNCTION_DECLARATOR);
        declarator = child_node->data.function_declarator.direct_declarator;
    }

    switch(declarator->kind) {
      case NODE_IDENTIFIER:
        identifier = declarator;
        break;
      case NODE_POINTER_DECLARATOR:
        identifier = get_function_name_from_children(declarator);
        break;
      case NODE_FUNCTION_DECLARATOR:
        identifier = get_function_name_from_children(declarator);
        break;
      default:
        ir_generation_num_errors++;
        printf("Function Name not found during function definition..\n");
        identifier = NULL;
        break;
    }
    return identifier;
}

void ir_generate_for_function_definition(struct node *function_definition) {
    /* Nothing to do with the function_def_specifier since it is not an expression.
     * We only care about the compound_statement */
  struct node *compound_statement = function_definition->data.function_definition.compound_statement;
  struct node *function_def_specifier = function_definition->data.function_definition.function_def_specifier;
  struct ir_instruction *function_begin, *function_end, *function_end_label;
  struct node *function_name_identifier;
  assert(NODE_FUNCTION_DEFINITION == function_definition->kind);

  function_name_identifier = get_function_name_from_children(function_def_specifier);

  function_begin = ir_instruction(IR_FUNCTION_BEGIN);
  ir_operand_identifier(function_begin, 0, function_name_identifier);

  function_definition->ir = ir_section(function_begin, function_begin);

  function_end_label = ir_instruction(IR_GENERATED_LABEL);
  ir_generate_label(function_end_label);

  ir_generate_for_expression(compound_statement, function_end_label, NULL);
  function_definition->ir = ir_concatenate(function_definition->ir,
                                           compound_statement->ir);

  
  function_end = ir_instruction(IR_FUNCTION_END);
  ir_operand_identifier(function_end, 0, function_name_identifier);

  ir_append(function_definition->ir, function_end_label);
  ir_append(function_definition->ir, function_end);
}

void ir_generate_for_while_statement(struct node *while_statement, 
                                     struct ir_instruction *function_end_label) {
  struct node *expression = while_statement->data.statement.expression;
  struct ir_instruction *label_instruction1, *label_instruction2;
  struct ir_instruction *gotoIfFalse_instruction, *goto_instruction;
  struct node *statement_within = while_statement->data.statement.statement;

  assert(NODE_STATEMENT == while_statement->kind);
  label_instruction1 = ir_instruction(IR_GENERATED_LABEL);
  ir_generate_label(label_instruction1);

  label_instruction2 = ir_instruction(IR_GENERATED_LABEL);
  ir_generate_label(label_instruction2);

  ir_generate_for_expression(expression, function_end_label, label_instruction2);

  gotoIfFalse_instruction = ir_instruction(IR_GOTO_IF_FALSE);
  ir_generate_gotoFalseOrTrue(gotoIfFalse_instruction, node_get_result(expression)->ir_operand, label_instruction2);

  ir_generate_for_expression(statement_within, function_end_label, label_instruction2);

  goto_instruction = ir_instruction(IR_GOTO);
  ir_generate_goto(goto_instruction, label_instruction1);

  while_statement->ir = ir_section(label_instruction1, label_instruction1);
  while_statement->ir = ir_concatenate(while_statement->ir, expression->ir);
  ir_append(while_statement->ir, gotoIfFalse_instruction);
  while_statement->ir = ir_concatenate(while_statement->ir, statement_within->ir);
  ir_append(while_statement->ir, goto_instruction);
  ir_append(while_statement->ir, label_instruction2);
}

void ir_generate_for_expr(struct node *expr) {
    struct node *expr1 = expr->data.expr.expr1;
    struct node *expr2 = expr->data.expr.expr2;
    switch(expr->data.expr.type_of_expr) {
      case ASSIGNMENT_EXPR:
        assert(expr1 == NULL);
        ir_generate_for_expression(expr2, NULL, NULL);
        expr->ir = expr2->ir;
        break;
      default:
        assert(0);
        break;
    }
}

void ir_generate_for_expression_statement(struct node *expression_statement,
                                          struct ir_instruction *function_end_label,
                                          struct ir_instruction *inner_loop_end_label) {
  struct ir_instruction *instruction;
  struct node *expression = expression_statement->data.statement.expression;
  assert(NODE_STATEMENT == expression_statement->kind);
  ir_generate_for_expression(expression, function_end_label, inner_loop_end_label);
  instruction = ir_instruction(IR_PRINT_NUMBER);
  ir_operand_copy(instruction, 0, node_get_result(expression)->ir_operand);

  expression_statement->ir = ir_copy(expression_statement->data.statement.expression->ir);
  ir_append(expression_statement->ir, instruction);
}

void ir_generate_for_return_statement(struct node *return_statement,
                                      struct ir_instruction *function_end_label) {
    struct ir_instruction *instruction, *goto_instruction;
  struct node *expression = return_statement->data.statement.expression;
  assert(NODE_STATEMENT == return_statement->kind);
  instruction = ir_instruction(IR_RETURN);
  if(expression != NULL) {
      ir_generate_for_expression(expression, NULL, NULL);
      ir_operand_copy(instruction, 0, node_get_result(expression)->ir_operand);
  } else {
      instruction->operands[0].kind = OPERAND_NULL;
  }

  if(expression != NULL) {
      return_statement->ir = ir_copy(expression->ir);
      ir_append(return_statement->ir, instruction);
  } else {
      return_statement->ir = ir_section(instruction, instruction);
  }

  goto_instruction = ir_instruction(IR_GOTO);
  ir_generate_goto(goto_instruction, function_end_label);
  
  ir_append(return_statement->ir, goto_instruction);
}

void ir_generate_for_for_expr(struct node *for_expr,
                              struct ir_instruction *label_instruction_conditional,
                              struct ir_instruction *label_instruction_unconditional) {
    struct node *initial_clause = for_expr->data.for_expr.initial_clause;
    struct node *expr1 = for_expr->data.for_expr.expr1;
    struct node *expr2 = for_expr->data.for_expr.expr2;

    struct ir_instruction *gotoIfFalse_instruction;
    assert(NODE_FOR_EXPR == for_expr->kind);

    if(initial_clause != NULL) {
        ir_generate_for_expression(initial_clause, NULL, NULL);
    }

    if(expr1 != NULL) {
        ir_generate_for_expression(expr1, NULL, NULL);
        gotoIfFalse_instruction = ir_instruction(IR_GOTO_IF_FALSE);
        ir_generate_gotoFalseOrTrue(gotoIfFalse_instruction, node_get_result(expr1)->ir_operand, label_instruction_conditional);
    }
    if(expr2 != NULL) {
        ir_generate_for_expression(expr2, NULL, NULL);
    }

    for_expr->ir = ir_section(label_instruction_unconditional, label_instruction_unconditional);
    if(initial_clause != NULL) {
        for_expr->ir = ir_concatenate(for_expr->ir, initial_clause->ir);
    }
    if(expr1 != NULL) {
        for_expr->ir = ir_concatenate(for_expr->ir, expr1->ir);
        ir_append(for_expr->ir, gotoIfFalse_instruction);
    }
    if(expr2 != NULL) {
        for_expr->ir = ir_concatenate(for_expr->ir, expr2->ir);
    }
}

void ir_generate_for_expression_list(struct node *expression_list, int *num_of_parameters) {
    struct node *expression_list_within = expression_list->data.expression_list.expression_list;
    struct node *assignment_expr = expression_list->data.expression_list.assignment_expr;
    struct ir_instruction *instruction;

    if(expression_list_within != NULL) {
        ir_generate_for_expression_list(expression_list_within, num_of_parameters);
        expression_list->ir = ir_copy(expression_list_within->ir);
    }
    ir_generate_for_expression(assignment_expr, NULL, NULL);
    ir_generate_for_conversion_to_rvalue(assignment_expr);
    instruction = ir_instruction(IR_FUNCTION_PARAMETER);

    instruction->operands[0].kind = OPERAND_NUMBER;
    instruction->operands[0].data.number = *num_of_parameters;
    instruction->operands[0].lvalue = false;
    (*num_of_parameters)++;

    ir_operand_copy(instruction, 1, node_get_result(assignment_expr)->ir_operand);

    if(expression_list->ir != NULL) {
        expression_list->ir = ir_concatenate(expression_list->ir, assignment_expr->ir);
        ir_append(expression_list->ir, instruction);
    } else {
        expression_list->ir = ir_copy(assignment_expr->ir);
        ir_append(expression_list->ir, instruction);
    }
}

void ir_generate_for_function_call(struct node *function_call) {
    struct node *postfix_expr = function_call->data.function_call.postfix_expr;
    struct node *expression_list = function_call->data.function_call.expression_list;
    struct ir_instruction *instruction = ir_instruction(IR_FUNCTION_CALL);
    struct ir_instruction *resultword_instruction = ir_instruction(IR_RESULTWORD);
    int num_of_parameters = 0; /* initialized value */
    assert(NODE_FUNCTION_CALL == function_call->kind);
    ir_generate_for_expression(postfix_expr, NULL, NULL);
    if(postfix_expr->kind != NODE_IDENTIFIER) {
        ir_generation_num_errors++;
        printf("ERROR: The function name has to be an identifier\n");
        return;
    }
    if(expression_list != NULL) {        
        ir_generate_for_expression_list(expression_list, &num_of_parameters);
        function_call->ir = ir_copy(expression_list->ir);
    }
    ir_operand_identifier(instruction, 0, postfix_expr);
    ir_append(function_call->ir, instruction);
    /* xxx: Need to implement ternary_operation's type assignment
     * if(node_get_result(function_call)->type->kind != TYPE_VOID) { */
        ir_operand_temporary(resultword_instruction, 0);
        ir_append(function_call->ir, resultword_instruction);
        node_get_result(function_call)->ir_operand = &resultword_instruction->operands[0];
        node_get_result(function_call)->ir_operand->lvalue = false;
    /* } else { */
    /*     node_get_result(function_call)->ir_operand = &instruction->operands[0]; */
    /*     node_get_result(function_call)->ir_operand->lvalue = false; */
    /* } */
}

void ir_generate_for_for_statement(struct node *for_statement,
                                   struct ir_instruction *function_end_label) {
  struct node *for_expr = for_statement->data.statement.expression;
  struct ir_instruction *label_instruction_unconditional, *label_instruction_conditional;
  struct ir_instruction *goto_instruction;
  struct node *statement_within = for_statement->data.statement.statement;

  assert(NODE_STATEMENT == for_statement->kind);

  label_instruction_unconditional = ir_instruction(IR_GENERATED_LABEL);
  ir_generate_label(label_instruction_unconditional);

  label_instruction_conditional = ir_instruction(IR_GENERATED_LABEL);
  ir_generate_label(label_instruction_conditional);

  ir_generate_for_for_expr(for_expr,
                           label_instruction_conditional,
                           label_instruction_unconditional);

  ir_generate_for_expression(statement_within, function_end_label, label_instruction_conditional);

  goto_instruction = ir_instruction(IR_GOTO);
  ir_generate_goto(goto_instruction, label_instruction_unconditional);

  for_statement->ir = ir_copy(for_expr->ir);
  for_statement->ir = ir_concatenate(for_statement->ir, for_expr->ir);
  for_statement->ir = ir_concatenate(for_statement->ir, statement_within->ir);
  ir_append(for_statement->ir, goto_instruction);
  ir_append(for_statement->ir, label_instruction_conditional);
}

void ir_generate_for_do_statement(struct node *do_statement,
                                  struct ir_instruction *function_end_label) {
  struct node *expression = do_statement->data.statement.expression;
  struct ir_instruction *label_instruction1, *label_instruction2;
  struct ir_instruction *gotoIfFalse_instruction, *goto_instruction;
  struct node *statement_within = do_statement->data.statement.statement;

  assert(NODE_STATEMENT == do_statement->kind);
  label_instruction1 = ir_instruction(IR_GENERATED_LABEL);
  ir_generate_label(label_instruction1);

  label_instruction2 = ir_instruction(IR_GENERATED_LABEL);
  ir_generate_label(label_instruction2);

  ir_generate_for_expression(statement_within, function_end_label, label_instruction2);

  ir_generate_for_expression(expression, NULL, NULL);
  gotoIfFalse_instruction = ir_instruction(IR_GOTO_IF_FALSE);
  ir_generate_gotoFalseOrTrue(gotoIfFalse_instruction, node_get_result(expression)->ir_operand, label_instruction2);

  goto_instruction = ir_instruction(IR_GOTO);
  ir_generate_goto(goto_instruction, label_instruction1);

  do_statement->ir = ir_section(label_instruction1, label_instruction1);
  do_statement->ir = ir_concatenate(do_statement->ir, statement_within->ir);
  do_statement->ir = ir_concatenate(do_statement->ir, expression->ir);
  ir_append(do_statement->ir, gotoIfFalse_instruction);
  ir_append(do_statement->ir, goto_instruction);
  ir_append(do_statement->ir, label_instruction2);
}

void ir_generate_for_if_statement(struct node *if_statement,
                                  struct ir_instruction *function_end_label,
                                  struct ir_instruction *inner_loop_end_label) {
    struct node *expr = if_statement->data.if_statement.expr;
    struct node *if_statement_within = if_statement->data.if_statement.if_statement;
    struct node *else_statement_within = if_statement->data.if_statement.else_statement;
    struct ir_instruction *label_instruction, *gotoIfFalse_instruction;
    assert(NODE_IF_STATEMENT == if_statement->kind);
    if(expr == NULL) {
        ir_generation_num_errors++;
        printf("ERROR: the Expression inside the if statement is empty. Not allowed\n");
    }
    ir_generate_for_expression(expr, NULL, NULL);
    label_instruction = ir_instruction(IR_GENERATED_LABEL);
    ir_generate_label(label_instruction);
    gotoIfFalse_instruction = ir_instruction(IR_GOTO_IF_FALSE);
    ir_generate_gotoFalseOrTrue(gotoIfFalse_instruction, node_get_result(expr)->ir_operand, label_instruction);
    if(if_statement_within != NULL) {
        ir_generate_for_expression(if_statement_within, function_end_label, inner_loop_end_label);
    }
    if(else_statement_within != NULL) {
        ir_generate_for_expression(else_statement_within, function_end_label, inner_loop_end_label);
    }
    if_statement->ir = ir_copy(expr->ir);
    ir_append(if_statement->ir, gotoIfFalse_instruction);
    if(if_statement_within != NULL) {
        if_statement->ir = ir_concatenate(if_statement->ir, if_statement_within->ir);
    }
    ir_append(if_statement->ir, label_instruction);
    if(else_statement_within != NULL) {
        if_statement->ir = ir_concatenate(if_statement->ir, else_statement_within->ir);
    }
}

void ir_generate_for_goto_statement(struct node *goto_statement) {
    struct node *identifier = goto_statement->data.statement.expression;
    struct ir_instruction *instruction;
    assert(NODE_STATEMENT == goto_statement->kind);
    instruction = ir_instruction(IR_GOTO);
    ir_operand_identifier(instruction, 0, identifier);
    goto_statement->ir = ir_section(instruction, instruction);
}

void ir_generate_for_statement(struct node *statement,
                                   struct ir_instruction *function_end_label,
                                   struct ir_instruction *inner_loop_end_label) {
    /* Reset temporaries for every statement */
    
  assert(NODE_STATEMENT == statement->kind);
  switch(statement->data.statement.type_of_statement) {
    case EXPRESSION_STATEMENT_TYPE:
      ir_generate_for_expression_statement(statement,
                                           function_end_label, 
                                           inner_loop_end_label);
      break;
    case WHILE_STATEMENT_TYPE:
      ir_generate_for_while_statement(statement, 
                                      function_end_label);
      break;
    case RETURN_STATEMENT_TYPE:
      ir_generate_for_return_statement(statement,
                                       function_end_label);
      break;
    case FOR_STATEMENT_TYPE:
      ir_generate_for_for_statement(statement,
                                    function_end_label);
      break;
    case DO_STATEMENT_TYPE:
      ir_generate_for_do_statement(statement,
                                   function_end_label);
      break;
    case GOTO_STATEMENT_TYPE:
      ir_generate_for_goto_statement(statement);
      break;
    default:
      printf("Type of statement: %d\n",
	     statement->data.statement.type_of_statement);
      assert(0);
      break;
  }
}

void ir_generate_for_statement_list(struct node *statement_list,
                                   struct ir_instruction *function_end_label,
                                   struct ir_instruction *inner_loop_end_label) {
  struct node *init = statement_list->data.statement_list.init;
  struct node *statement = statement_list->data.statement_list.statement;

  assert(NODE_STATEMENT_LIST == statement_list->kind);

  if (NULL != init) {
      ir_generate_for_expression(init, function_end_label, inner_loop_end_label);
      ir_generate_for_expression(statement, function_end_label, inner_loop_end_label);
    statement_list->ir = ir_concatenate(init->ir, statement->ir);
  } else {
      ir_generate_for_expression(statement, function_end_label, inner_loop_end_label);
    statement_list->ir = statement->ir;
  }
}

void ir_generate_for_expression(struct node *expression,
                                   struct ir_instruction *function_end_label,
                                   struct ir_instruction *inner_loop_end_label) {
    struct ir_instruction *no_op_instruction;
  switch (expression->kind) {
    case NODE_IDENTIFIER:
      ir_generate_for_identifier(expression);
      break;

    case NODE_NUMBER:
      ir_generate_for_number(expression);
      break;
    case NODE_STRING:
      ir_generate_for_string(expression);
      break;
    case NODE_BINARY_OPERATION:
      ir_generate_for_binary_operation(expression);
      break;
    case NODE_TERNARY_OPERATION:
      ir_generate_for_ternary_operation(expression);
      break;
    case NODE_UNARY_OPERATION:
      ir_generate_for_unary_operation(expression);
      break;

    case NODE_DECL:
      no_op_instruction = ir_instruction(IR_NO_OPERATION);
      expression->ir = ir_section(no_op_instruction,
				  no_op_instruction);
      /* nothing to do for declarations */
      break;
    case NODE_FUNCTION_DEFINITION:
      ir_generate_for_function_definition(expression);
      next_temporary = 0;
      break;
   case NODE_COMPOUND_STATEMENT:
     ir_generate_for_compound_statement(expression, function_end_label, inner_loop_end_label);
      break;
    case NODE_STATEMENT:
      ir_generate_for_statement(expression, function_end_label, inner_loop_end_label);
      next_temporary = 0;
      break;
    case NODE_STATEMENT_LIST:
      ir_generate_for_statement_list(expression, function_end_label, inner_loop_end_label);
      break;
    case NODE_EXPR:
      ir_generate_for_expr(expression);
      break;
    case NODE_FOR_EXPR:
      printf("For_expr should only be called from FOR statement\n");
      assert(0);
      break;
    case NODE_IF_STATEMENT:
      ir_generate_for_if_statement(expression, function_end_label, inner_loop_end_label);
      break;
    case NODE_FUNCTION_CALL:
      ir_generate_for_function_call(expression);
      break;
    case NODE_EXPRESSION_LIST:
      printf("Expression List should only be called from function call's IR Generator\n");
      assert(0);
      break;
    case NODE_CAST_EXPR:
      ir_generate_for_cast_expr(expression);
      break;
    default:
      printf("Node not found: %d\n", expression->kind);
      assert(0);
      break;
  }
}

void ir_generate_for_translation_unit(struct node *translation_unit) {
  struct node *top_level_decl = translation_unit->data.translation_unit.top_level_decl;
  struct node *translation_unit_within = translation_unit->data.translation_unit.translation_unit;

  assert(NODE_TRANSLATION_UNIT == translation_unit->kind);
  if (NULL != translation_unit_within) {
      printf("Translation Unit kind: %d\n", translation_unit_within->kind);
    ir_generate_for_translation_unit(translation_unit_within);
    printf("Top level decl kind: %d\n", top_level_decl == NULL);
    ir_generate_for_expression(top_level_decl, NULL, NULL);
    translation_unit->ir = ir_concatenate(translation_unit_within->ir, top_level_decl->ir);
  } else {
      ir_generate_for_expression(top_level_decl, NULL, NULL);
    translation_unit->ir = top_level_decl->ir;
  }
}

void ir_generate_for_program(struct node *program) {
    struct node *top_level_decl = program->data.translation_unit.top_level_decl;
    struct node *translation_unit_within = program->data.translation_unit.translation_unit;

    assert(NODE_TRANSLATION_UNIT == program->kind);
    assert(top_level_decl == NULL);
    assert(translation_unit_within != NULL);
    ir_generate_for_translation_unit(translation_unit_within);
    program->ir = translation_unit_within->ir;
}

/**********************
 * PRINT INSTRUCTIONS *
 **********************/

static void ir_print_opcode(FILE *output, int kind) {
  static char *instruction_names[] = {
    NULL,
    "NOP",
    "MULT",
    "DIV",
    "ADD",
    "SUB",
    "REM",
    "LI",
    "COPY",
    "PNUM",
    "GENLABEL",
    "GOTO",
    "FCNCALL",
    "ADDRESSOF",
    "LOADWORD",
    "LESSTHAN",
    "LTOREQTO",
    "GRTHAN",
    "GTOREQTO",
    "SHIFLEFT",
    "SHIFRIGHT",
    "EQTO",
    "NOTEQTO",
    "BITOR",
    "BITXOR",
    "BITAND",
    "BIFEQZ",
    "BIFNOTEQZ",
    "STORWORD",
    "SIZEOF",
    "BITWISENOT",
    "LOGICALNOT",
    "NEGATION",
    "GOTOIFALSE",
    "RETURNWORD",
    "PROCBEGIN",
    "PROCEND",
    "GOTOIFTRUE",
    "CASTUWORD",
    "CASTSWORD",
    "CASTUHWORD",
    "CASTSHWORD",
    "CASTUBYTE",
    "CASTSBYTE",
    "PARAMETER",
    "RESULWORD",
    NULL
  };

  fprintf(output, "%-10s", instruction_names[kind]);
}

static void ir_print_operand(FILE *output, struct ir_operand *operand) {
  switch (operand->kind) {
    case OPERAND_NUMBER:
      fprintf(output, "%10hu", (unsigned short)operand->data.number);
      break;

    case OPERAND_TEMPORARY:
      fprintf(output, "     t%04d", operand->data.temporary);
      break;
    case OPERAND_IDENTIFIER:
      fprintf(output, "     %5s", operand->data.identifier_name);
      break;
    case OPERAND_GENERATED_LABEL:
      fprintf(output, "     __GeneratedLabel_%04d", operand->data.generated_label);
      break;
    case OPERAND_STRING:
      fprintf(output, "     __GeneratedStringLabel_%04d", operand->data.string_label.generated_label);
      break;
    case OPERAND_NULL:
      break;
    default:
      fprintf(output, "Unknown operand type found: %d\n", operand->kind);
      assert(0);
      break;
  }
}

void ir_print_instruction(FILE *output, struct ir_instruction *instruction) {
  ir_print_opcode(output, instruction->kind);

  switch (instruction->kind) {
    case IR_MULTIPLY:
    case IR_DIVIDE:
    case IR_ADD:
    case IR_SUBTRACT:
    case IR_LESS_THAN:
    case IR_LESS_THAN_OR_EQ_TO:
    case IR_GREATER_THAN:
    case IR_GREATER_THAN_OR_EQ_TO:
    case IR_SHIFT_LEFT:
    case IR_SHIFT_RIGHT:
    case IR_EQUAL_TO:
    case IR_NOT_EQUAL_TO:
    case IR_BITWISE_OR:
    case IR_BITWISE_XOR:
    case IR_BITWISE_AND:
      ir_print_operand(output, &instruction->operands[0]);
      fprintf(output, ", ");
      ir_print_operand(output, &instruction->operands[1]);
      fprintf(output, ", ");
      ir_print_operand(output, &instruction->operands[2]);
      break;
    case IR_LOAD_IMMEDIATE:
    case IR_COPY:
    case IR_ADDRESS_OF:
    case IR_LOAD_WORD:
    case IR_STORE_WORD:
    case IR_BIFEQZ:
    case IR_BIFNOTEQZ:
    case IR_BITWISE_NOT:
    case IR_SIZEOF:
    case IR_LOGICAL_NOT:
    case IR_NEGATION:
    case IR_GOTO_IF_FALSE:
    case IR_GOTO_IF_TRUE:
    case IR_CAST_TO_U_WORD:
    case IR_CAST_TO_S_WORD:
    case IR_CAST_TO_U_HALFWORD:
    case IR_CAST_TO_S_HALFWORD:
    case IR_CAST_TO_U_BYTE:
    case IR_CAST_TO_S_BYTE:
    case IR_FUNCTION_PARAMETER:
      ir_print_operand(output, &instruction->operands[0]);
      fprintf(output, ", ");
      ir_print_operand(output, &instruction->operands[1]);
      break;
    case IR_GENERATED_LABEL:
    case IR_PRINT_NUMBER:
    case IR_GOTO:
    case IR_RETURN:
    case IR_FUNCTION_CALL:
    case IR_FUNCTION_BEGIN:
    case IR_FUNCTION_END:
    case IR_RESULTWORD:
      ir_print_operand(output, &instruction->operands[0]);
      break;
    case IR_NO_OPERATION:
      break;
    default:
      printf("Unknown instruction found: %d\n", instruction->kind);
      assert(0);
      break;
  }
}

void ir_print_section(FILE *output, struct ir_section *section) {
  int i = 0;
  struct ir_instruction *iter = section->first;
  struct ir_instruction *prev = NULL;
  while (NULL != iter && section->last != prev) {
    fprintf(output, "%5d     ", i++);
    ir_print_instruction(output, iter);
    fprintf(output, "\n");
    iter = iter->next;
  }
}
