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

struct type *type_basic(bool is_unsigned, int width) {
  struct type *basic;

  basic = malloc(sizeof(struct type));
  assert(NULL != basic);

  basic->kind = TYPE_BASIC;
  basic->data.basic.is_unsigned = is_unsigned;
  basic->data.basic.width = width;
  return basic;
}

struct type *type_void() {
  struct type *void_type;
  void_type = malloc(sizeof(struct type));
  assert(NULL != void_type);

  void_type->kind = TYPE_VOID;
  return void_type;
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
    /* xxx : Need to fix the following.. */
    function_type->data.function.parameter_list = NULL;
    function_type->data.function.function_symbol_table = NULL;
    function_type->data.function.function_body = NULL;
    return function_type;
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
  assert(type_is_equal(node_get_result(binary_operation->data.binary_operation.left_operand)->type,
                       node_get_result(binary_operation->data.binary_operation.right_operand)->type));
  binary_operation->data.binary_operation.result.type =
    node_get_result(binary_operation->data.binary_operation.left_operand)->type;
}

void type_convert_assignment(struct node *binary_operation) {
  assert(NODE_BINARY_OPERATION == binary_operation->kind);
  assert(type_is_equal(node_get_result(binary_operation->data.binary_operation.left_operand)->type,
                       node_get_result(binary_operation->data.binary_operation.right_operand)->type));
  binary_operation->data.binary_operation.result.type =
    node_get_result(binary_operation->data.binary_operation.left_operand)->type;
}

void type_assign_in_unary_operation(struct node *unary_operation) {
  assert(NODE_UNARY_OPERATION == unary_operation->kind);
  type_assign_in_expression(unary_operation->data.unary_operation.the_operand);
  /* No converting or type checking yet */
}

void type_assign_in_binary_operation(struct node *binary_operation) {
  assert(NODE_BINARY_OPERATION == binary_operation->kind);
  type_assign_in_expression(binary_operation->data.binary_operation.left_operand);
  type_assign_in_expression(binary_operation->data.binary_operation.right_operand);

  switch (binary_operation->data.binary_operation.operation) {
    case BINOP_MULTIPLICATION:
    case BINOP_DIVISION:
    case BINOP_ADDITION:
    case BINOP_SUBTRACTION:
      type_convert_usual_binary(binary_operation);
      break;

    case BINOP_ASSIGN:
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
        expression->data.identifier.symbol->result.type = type_basic(false, TYPE_WIDTH_INT);
      }
      break;

    case NODE_NUMBER:
      expression->data.number.result.type = type_basic(false, TYPE_WIDTH_INT);
      break;

    case NODE_BINARY_OPERATION:
      type_assign_in_binary_operation(expression);
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
