#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "node.h"
#include "symbol.h"
#include "type.h"

/**************************
 * PRINT TYPE EXPRESSIONS *
 **************************/

void type_print_basic(FILE *output, struct type *basic) {
  assert(TYPE_BASIC == basic->kind);

  if (basic->data.basic.is_unsigned) {
    fputs("unsigned ", output);
  } else {
    fputs("signed ", output);
  }

  switch (basic->data.basic.width) {
    case TYPE_WIDTH_INT:
      fputs("int", output);
      break;
    default:
      assert(0);
      break;
  }
}

void type_print(FILE *output, struct type *kind) {
  assert(NULL != kind);

  switch (kind->kind) {
    case TYPE_BASIC:
      type_print_basic(output, kind);
      break;
    default:
      assert(0);
      break;
  }
}

/***************************
 * CREATE TYPE EXPRESSIONS *
 ***************************/

struct type *type_basic(bool is_unsigned, int width, int conversion_rank) {
  struct type *basic;

  basic = malloc(sizeof(struct type));
  assert(NULL != basic);

  basic->kind = TYPE_BASIC;
  basic->data.basic.is_unsigned = is_unsigned;
  basic->data.basic.width = width;
  basic->data.basic.conversion_rank = conversion_rank;
  return basic;
}

struct type *type_void() {
  struct type *void_type;
  void_type = malloc(sizeof(struct type));
  assert(NULL != void_type);

  void_type->kind = TYPE_VOID;
  return void_type;
}

struct type *type_label() {
  struct type *label_type;
  label_type = malloc(sizeof(struct type));
  assert(NULL != label_type);

  label_type->kind = TYPE_LABEL;
  return label_type;
}

struct type *type_pointer(struct type *pointee) {
  struct type *pointer_type;
  pointer_type = malloc(sizeof(struct type));
  assert(NULL != pointer_type);

  pointer_type->kind = TYPE_POINTER;
  pointer_type->data.pointer.pointee = pointee;
  return pointer_type;
}

struct type *type_function(struct type *type) {
    struct type *function_type;
    function_type = malloc(sizeof(struct type));
    assert(NULL != function_type);

    function_type->kind = TYPE_FUNCTION;
    function_type->data.function.return_type = type;
    function_type->data.function.parameter_list = NULL;
    function_type->data.function.function_symbol_table = NULL;
    function_type->data.function.function_body = NULL;
    function_type->data.function.number_of_parameters = 0;
    return function_type;
}


struct type *type_array(struct type *type,
                        unsigned long array_size) {
    struct type *array_type;
    array_type = malloc(sizeof(struct type));
    assert(NULL != array_type);

    array_type->kind = TYPE_ARRAY;
    array_type->data.array.array_type = type;
    array_type->data.array.array_size = array_size;
    return array_type;
}

/****************************************
 * TYPE EXPRESSION INFO AND COMPARISONS *
 ****************************************/

int type_is_equal(struct type *left, struct type *right) {
  int equal;

  equal = left->kind == right->kind;

  if (equal) {
    switch (left->kind) {
      case TYPE_BASIC:
        equal = equal && left->data.basic.is_unsigned == right->data.basic.is_unsigned;
        equal = equal && left->data.basic.width == right->data.basic.width;
        break;
      default:
        equal = 0;
        break;
    }
  }

  return equal;
}

int type_is_arithmetic(struct type *t) {
  return TYPE_BASIC == t->kind;
}

int type_is_unsigned(struct type *t) {
  return type_is_arithmetic(t) && t->data.basic.is_unsigned;
}

int type_is_void(struct type *t) {
  return TYPE_VOID == t->kind;
}

int type_is_scalar(struct type *t) {
  return type_is_arithmetic(t) || TYPE_POINTER == t->kind;
}

int node_is_lvalue(struct node *n) {
  assert(NULL != n);
  return NODE_IDENTIFIER == n->kind;
}

int type_size(struct type *t) {
  switch (t->kind) {
    case TYPE_BASIC:
      return t->data.basic.width;
    case TYPE_POINTER:
      return TYPE_WIDTH_POINTER;
    default:
      return 0;
  }
}

/*****************
 * TYPE CHECKING *
 *****************/

int type_checking_num_errors;

void type_assign_in_expression(struct node *expression);

void type_convert_usual_binary(struct node *binary_operation) {
  assert(NODE_BINARY_OPERATION == binary_operation->kind);
  if(type_is_equal(node_get_result(binary_operation->data.binary_operation.left_operand)->type,
                   node_get_result(binary_operation->data.binary_operation.right_operand)->type)) {
      binary_operation->data.binary_operation.result.type =
          node_get_result(binary_operation->data.binary_operation.left_operand)->type;
  } else {
      /* struct type *left_operand = node_get_result(binary_operation->data.binary_operation.left_operand)->type; */
      /* struct type *right_operand = node_get_result(binary_operation->data.binary_operation.right_operand)->type; */
      
      type_checking_num_errors++; printf("ERROR!\n");
  }
}

void type_convert_assignment(struct node *binary_operation) {
  assert(NODE_BINARY_OPERATION == binary_operation->kind);
  assert(type_is_equal(node_get_result(binary_operation->data.binary_operation.left_operand)->type,
                       node_get_result(binary_operation->data.binary_operation.right_operand)->type));
  binary_operation->data.binary_operation.result.type =
    node_get_result(binary_operation->data.binary_operation.left_operand)->type;
}

void type_assign_in_unary_operation(struct node *unary_operation) {
  struct node *the_operand = unary_operation->data.unary_operation.the_operand;
  struct type *operand_type = node_get_result(the_operand)->type;
  assert(NODE_UNARY_OPERATION == unary_operation->kind);
  switch(operand_type->kind) {
    case TYPE_BASIC:
    case TYPE_VOID:
    case TYPE_POINTER:
      node_get_result(unary_operation)->type = node_get_result(the_operand)->type;
      break;
    case TYPE_ARRAY:
      node_get_result(unary_operation)->type = type_pointer(
          node_get_result(the_operand)->type->data.array.array_type);
      break;
    case TYPE_FUNCTION:
      type_checking_num_errors++; printf("ERROR: operand of unary operation cannot be function type\n");
      break;
    case TYPE_LABEL:
      type_checking_num_errors++; printf("ERROR: operand of unary operation cannot be label type\n");
      break;
    default:
      type_checking_num_errors++; printf("ERROR: the operand of unary operand is of unknown type\n");
      break;
  }
  /* xxx: No converting yet */
}

struct type * apply_usual_arithmetic_binary_conversion(struct type *left, 
                                                       struct type *right) {
    int conversion_rank = (left->data.basic.conversion_rank >= right->data.basic.conversion_rank)?
        left->data.basic.conversion_rank:right->data.basic.conversion_rank;
    int width = TYPE_WIDTH_INT;
    bool are_both_unsigned = (left->data.basic.is_unsigned) && (right->data.basic.is_unsigned);
    bool are_both_signed = (!left->data.basic.is_unsigned) && (!right->data.basic.is_unsigned);
    bool is_unsigned = false;

    switch (conversion_rank) {
      case CONVERSION_RANK_LONG:
        width = TYPE_WIDTH_LONG;
        break;
      case CONVERSION_RANK_INT:
        width = TYPE_WIDTH_INT;
      case CONVERSION_RANK_SHORT:
        width = TYPE_WIDTH_SHORT;
      case CONVERSION_RANK_CHAR:
        width = TYPE_WIDTH_CHAR;
    }

    if(are_both_unsigned || are_both_signed) {
        is_unsigned = are_both_unsigned;
    } else {
        /* This means that one of them is signed and the other unsigned.
           The signed-ness is determined by the conversion rank
         */
        if(left->data.basic.conversion_rank == right->data.basic.conversion_rank) {
            is_unsigned = true;
        } else {
            is_unsigned = (left->data.basic.conversion_rank > right->data.basic.conversion_rank)?
                left->data.basic.is_unsigned: right->data.basic.is_unsigned;
        }
        /* xxx: One operand is unsigned and the other is signed with greater rank but cannot
         * represent all values of the unsigned type - Need to implement this case
         */
    }
    /* Convert all the operands to the 'converted' type */
    left = type_basic(is_unsigned, width, conversion_rank);
    right = type_basic(is_unsigned, width, conversion_rank);
    
    return type_basic(is_unsigned, width, conversion_rank);
}

void type_convert_multiplicative(struct node *binary_operation) {
  struct type *left_operand_type = node_get_result(binary_operation->data.binary_operation.left_operand)->type;
  struct type *right_operand_type = node_get_result(binary_operation->data.binary_operation.right_operand)->type;
  assert(NODE_BINARY_OPERATION == binary_operation->kind);
  if(type_is_arithmetic(left_operand_type) && type_is_arithmetic(right_operand_type)) {
      node_get_result(binary_operation)->type = 
          apply_usual_arithmetic_binary_conversion(left_operand_type, 
                                                   right_operand_type);
  } else {
      type_checking_num_errors++; printf("ERROR: operands of multiplicative expr are not arithmetic\n");
  }

  if(type_is_equal(node_get_result(binary_operation->data.binary_operation.left_operand)->type,
                   node_get_result(binary_operation->data.binary_operation.right_operand)->type)) {
      binary_operation->data.binary_operation.result.type =
          node_get_result(binary_operation->data.binary_operation.left_operand)->type;
  } else {
      
      type_checking_num_errors++; printf("ERROR!\n");
  }
}

void type_assign_in_binary_operation(struct node *binary_operation) {
  assert(NODE_BINARY_OPERATION == binary_operation->kind);
  type_assign_in_expression(binary_operation->data.binary_operation.left_operand);
  type_assign_in_expression(binary_operation->data.binary_operation.right_operand);

  switch (binary_operation->data.binary_operation.operation) {
    case BINOP_MULTIPLICATION:
    case BINOP_DIVISION:
      type_convert_multiplicative(binary_operation);
      break;
    case BINOP_ADDITION:
    case BINOP_SUBTRACTION:
    case BINOP_REMAINDER:
      type_convert_usual_binary(binary_operation);
      break;

    case BINOP_ASSIGN:
    case BINOP_ASSIGN_PLUS_EQUAL:
    case BINOP_ASSIGN_MINUS_EQUAL:
    case BINOP_ASSIGN_ASTERISK_EQUAL:
    case BINOP_ASSIGN_SLASH_EQUAL:
    case BINOP_ASSIGN_PERCENT_EQUAL:
    case BINOP_ASSIGN_LESS_LESS_EQUAL:
    case BINOP_ASSIGN_GREATER_GREATER_EQUAL:
    case BINOP_ASSIGN_AMPERSAND_EQUAL:
    case BINOP_ASSIGN_CARET_EQUAL:
    case BINOP_ASSIGN_VBAR_EQUAL:
      type_convert_assignment(binary_operation);
      break;

    default:
      assert(0);
      break;
  }
}


void type_assign_in_expression(struct node *expression) {
  switch (expression->kind) {
    case NODE_UNARY_OPERATION:
      type_assign_in_unary_operation(expression);
      break;

    case NODE_IDENTIFIER:
      if (NULL == expression->data.identifier.symbol->result.type) {
          expression->data.identifier.symbol->result.type = 
              type_basic(false, TYPE_WIDTH_INT, CONVERSION_RANK_INT); /* xxx: 3rd argument Needs to change */
      }
      break;

    case NODE_NUMBER:
      expression->data.number.result.type = 
          type_basic(false, TYPE_WIDTH_INT, CONVERSION_RANK_INT);  /* xxx: 3rd argument Needs to change */
      break;

    case NODE_BINARY_OPERATION:
      type_assign_in_binary_operation(expression);
      break;
    case NODE_STRING:
      expression->data.number.result.type = 
          type_pointer(type_basic(false, TYPE_WIDTH_CHAR, CONVERSION_RANK_CHAR));
      break;
    case NODE_TERNARY_OPERATION:
      /* xxx: type_assign_in_ternary_operation(expression); */
      break;
    case NODE_STATEMENT:
      /* xxx: type_assign_in_statement(expression); */
      break;
    case NODE_STATEMENT_LIST:
      type_assign_in_statement_list(expression);
      break;
    case NODE_DECL:
      break;
    case NODE_TYPE_SPECIFIER:
      break;
    case NODE_POINTER:
      break;
    case NODE_EXPR:
      break;
    case NODE_ABSTRACT_DECL:
      break;
    case NODE_FOR_EXPR:
      break;
    case NODE_TRANSLATION_UNIT:
      break;
    case NODE_IF_STATEMENT:
      break;
    case NODE_POINTER_DECLARATOR:
      break;
    case NODE_PARAMETER_DECL:
      break;
    case NODE_FUNCTION_DEF_SPECIFIER:
      break;
    case NODE_FUNCTION_DECLARATOR:
      break;
    case NODE_PARAMETER_LIST:
      break;
    case NODE_ARRAY_DECLARATOR:
      break;
    case NODE_LABELED_STATEMENT:
      break;
    case NODE_COMPOUND_STATEMENT:
      break;
    case NODE_FUNCTION_DEFINITION:
      break;
    default:
      assert(0);
      break;
  }
}

void type_assign_in_expression_statement(struct node *expression_statement) {
  assert(NODE_STATEMENT == expression_statement->kind);
  type_assign_in_expression(expression_statement->data.statement.expression);
}

void type_assign_in_statement_list(struct node *statement_list) {
  assert(NODE_STATEMENT_LIST == statement_list->kind);
  if (NULL != statement_list->data.statement_list.init) {
    type_assign_in_statement_list(statement_list->data.statement_list.init);
  }
  type_assign_in_expression_statement(statement_list->data.statement_list.statement);
}

void type_assign_in_translation_unit(struct node *translation_unit) {
  assert(NODE_TRANSLATION_UNIT == translation_unit->kind);
  if (NULL != translation_unit->data.translation_unit.translation_unit) {
    type_assign_in_translation_unit(translation_unit->data.translation_unit.translation_unit);
  }
  if(NULL != translation_unit->data.translation_unit.top_level_decl) {
      type_assign_in_expression(translation_unit->data.translation_unit.top_level_decl);
  }
}
