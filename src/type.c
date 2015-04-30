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

int type_is_pointer(struct type *t) {
  return TYPE_POINTER == t->kind;
}

int type_is_array(struct type *t) {
  return TYPE_ARRAY == t->kind;
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

void add_cast_of_basic_type(struct node *expression, bool is_unsigned,
                            int width, int conversion_rank) {
    struct node *type_specifier;
    struct node *cast_expr;
    switch(conversion_rank) {
      case CONVERSION_RANK_LONG:
        type_specifier = is_unsigned?
            node_type_specifier(UNSIGNED_LONG_INT):
        node_type_specifier(SIGNED_LONG_INT);
        break;
      case CONVERSION_RANK_INT:
        type_specifier = is_unsigned?
            node_type_specifier(UNSIGNED_INT):
        node_type_specifier(SIGNED_INT);
        break;
      case CONVERSION_RANK_SHORT:
        type_specifier = is_unsigned?
            node_type_specifier(UNSIGNED_SHORT_INT):
        node_type_specifier(SIGNED_SHORT_INT);
        break;
      case CONVERSION_RANK_CHAR:
        type_specifier = is_unsigned?
            node_type_specifier(UNSIGNED_CHARACTER_TYPE):
        node_type_specifier(SIGNED_CHARACTER_TYPE);
        break;
      default:
        assert(0); printf("ERROR: Unknown conversion rank found\n");
        break;
    }
    cast_expr = node_cast_expr(type_specifier, expression);
    node_get_result(cast_expr)->type = type_basic(is_unsigned, width, conversion_rank);
    expression = cast_expr;
}

void add_cast_of_void_type(struct node *expression) {
    struct node *type_specifier = node_type_specifier(VOID_TYPE);
    struct node *cast_expr = node_cast_expr(type_specifier, expression);
    node_get_result(cast_expr)->type = type_void();
    expression = cast_expr;
}

void add_cast_expr(struct node *expression, struct type *type) {
    switch(type->kind) {
      case TYPE_BASIC:
        add_cast_of_basic_type(expression, type->data.basic.is_unsigned,
                               type->data.basic.width, type->data.basic.conversion_rank);
        break;
      case TYPE_VOID:
        add_cast_of_void_type(expression);
        break;
      case TYPE_POINTER:
        break;
      case TYPE_ARRAY:
        /* xxx add_cast_of_pointer_type_to_array(expression, type);  */
        break;
      case TYPE_FUNCTION:
        assert(0); printf("ERROR: Can't add cast of type function\n'");
        break;
      case TYPE_LABEL:
        assert(0); printf("ERROR: Can't add cast of type label\n'");
        break;
      default:
        assert(0); printf("ERROR: Unknown type found\n");
        break;
    }
}

void type_convert_usual_binary(struct node *binary_operation) {
    assert(NODE_BINARY_OPERATION == binary_operation->kind);

    assert(type_is_equal(node_get_result(binary_operation->data.binary_operation.left_operand)->type,
                         node_get_result(binary_operation->data.binary_operation.right_operand)->type));
    binary_operation->data.binary_operation.result.type =
        node_get_result(binary_operation->data.binary_operation.left_operand)->type;
}

void type_convert_assignment(struct node *binary_operation) {
    assert(NODE_BINARY_OPERATION == binary_operation->kind);
    /* xxx: assert(type_is_equal(node_get_result(binary_operation->data.binary_operation.left_operand)->type, */
    /*                      node_get_result(binary_operation->data.binary_operation.right_operand)->type)); */
    if(type_is_equal(node_get_result(binary_operation->data.binary_operation.left_operand)->type,
                     node_get_result(binary_operation->data.binary_operation.right_operand)->type)) {
        binary_operation->data.binary_operation.result.type =
            node_get_result(binary_operation->data.binary_operation.left_operand)->type;
    }
}

void apply_usual_arithmetic_unary_conversion(struct node *unary_operation) {
    struct node *the_operand = unary_operation->data.unary_operation.the_operand;
    struct type *operand_type = node_get_result(the_operand)->type;
    struct type *conversion_type = type_basic(false, TYPE_WIDTH_INT, CONVERSION_RANK_INT);

    assert(NODE_UNARY_OPERATION == unary_operation->kind);
    assert(operand_type->kind == TYPE_BASIC);
    if(operand_type->data.basic.conversion_rank < CONVERSION_RANK_INT) {
        add_cast_expr(the_operand, conversion_type);
    } else {
        /* No conversion needed */
    }
}

void apply_usual_array_unary_conversion(struct node *unary_operation) {
    struct node *the_operand = unary_operation->data.unary_operation.the_operand;
    struct type *operand_type = node_get_result(the_operand)->type;
    struct type *conversion_type = type_pointer(operand_type->data.array.array_type);

    assert(NODE_UNARY_OPERATION == unary_operation->kind);
    assert(operand_type->kind == TYPE_ARRAY);
    printf("Converting array to pointer.. \n");
    add_cast_expr(the_operand, conversion_type);
    node_get_result(unary_operation)->type = type_pointer(
        node_get_result(the_operand)->type->data.array.array_type);
    /* After this step, we should not come across array type anywhere.. */
}

void type_assign_in_unary_operation(struct node *unary_operation) {
  struct node *the_operand = unary_operation->data.unary_operation.the_operand;
  struct type *operand_type;
  type_assign_in_expression(the_operand);
  operand_type = node_get_result(the_operand)->type;

  assert(NODE_UNARY_OPERATION == unary_operation->kind);
  switch(operand_type->kind) {
    case TYPE_BASIC:
      apply_usual_arithmetic_unary_conversion(unary_operation);
      node_get_result(unary_operation)->type =
          node_get_result(unary_operation->data.unary_operation.the_operand)->type;
      break;
    case TYPE_VOID:
    case TYPE_POINTER:
      node_get_result(unary_operation)->type = node_get_result(the_operand)->type;
      break;
    case TYPE_ARRAY:
      apply_usual_array_unary_conversion(unary_operation);
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
        break;
      case CONVERSION_RANK_SHORT:
        assert(0); printf("ERROR: Conversion rank should be INT or higher\n");
        break;
      case CONVERSION_RANK_CHAR:
        assert(0); printf("ERROR: Conversion rank should be INT or higher\n");
        break;
      default:
        assert(0);
        break;
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
    }

    return type_basic(is_unsigned, width, conversion_rank);
}

/* If the operands are either a pointer or an integer, the result's type should be
 * boolean. Since we don't have a boolean type, we will use an int type i.e.,
 * TYPE_BASIC
 */
void type_convert_scalar(struct node *binary_operation) {
  struct type *left_operand_type = node_get_result(binary_operation->data.binary_operation.left_operand)->type;
  struct type *right_operand_type = node_get_result(binary_operation->data.binary_operation.right_operand)->type;
  assert(NODE_BINARY_OPERATION == binary_operation->kind);
  if (type_is_scalar(left_operand_type) && type_is_scalar(right_operand_type)) {
      node_get_result(binary_operation)->type = type_basic(true, TYPE_WIDTH_INT, CONVERSION_RANK_INT);
  } else {
      type_checking_num_errors++; printf("ERROR: operands of expression need to be arithmetic or a pointer\n");
  }
}

/* This function adds the implicit casts to the operands and then sets the type
 * on the type of the binary_operation node
 */
void type_convert_arithmetic(struct node *binary_operation) {
  struct type *left_operand_type = node_get_result(binary_operation->data.binary_operation.left_operand)->type;
  struct type *right_operand_type = node_get_result(binary_operation->data.binary_operation.right_operand)->type;
  struct type *result_type;
  assert(NODE_BINARY_OPERATION == binary_operation->kind);
  if(type_is_arithmetic(left_operand_type) && type_is_arithmetic(right_operand_type)) {
      result_type =
          apply_usual_arithmetic_binary_conversion(left_operand_type,
                                                   right_operand_type);
      add_cast_expr(binary_operation->data.binary_operation.left_operand, result_type);
      add_cast_expr(binary_operation->data.binary_operation.right_operand, result_type);
      type_convert_usual_binary(binary_operation);
  } else {
      type_checking_num_errors++; printf("ERROR: operands of expression need to be arithmetic\n");
  }
}

void type_convert_additive(struct node *binary_operation) {
  struct type *left_operand_type = node_get_result(binary_operation->data.binary_operation.left_operand)->type;
  struct type *right_operand_type = node_get_result(binary_operation->data.binary_operation.right_operand)->type;
  struct type *result_type;
  assert(NODE_BINARY_OPERATION == binary_operation->kind);
  if(type_is_arithmetic(left_operand_type) && type_is_arithmetic(right_operand_type)) {
      result_type =
          apply_usual_arithmetic_binary_conversion(left_operand_type,
                                                   right_operand_type);
      add_cast_expr(binary_operation->data.binary_operation.left_operand, result_type);
      add_cast_expr(binary_operation->data.binary_operation.right_operand, result_type);
      type_convert_usual_binary(binary_operation);
  } else if (type_is_pointer(left_operand_type) && type_is_arithmetic(right_operand_type)) {
      node_get_result(binary_operation)->type = left_operand_type;
  } else if (type_is_arithmetic(left_operand_type) && type_is_pointer(right_operand_type)) {
    switch(binary_operation->data.binary_operation.operation) {
    case BINOP_ADDITION:
      node_get_result(binary_operation)->type = right_operand_type;
      break;
    case BINOP_SUBTRACTION:
      type_checking_num_errors++; printf("ERROR: operands of the subtraction expr are not compatible\n");
      break;
    default:
      type_checking_num_errors++; printf("ERROR: operands of the additive expr are not compatible\n");
      break;
    }
  } else if(type_is_array(left_operand_type) && type_is_arithmetic(right_operand_type)) {
      assert(binary_operation->data.binary_operation.operation == BINOP_ADDITION);
      node_get_result(binary_operation)->type = left_operand_type->data.array.array_type;
  } else if(type_is_pointer(left_operand_type) && type_is_pointer(right_operand_type)) {
    /* xxx: How to set a type for the pointer in this case? */
    type_checking_num_errors++; printf("ERROR: operands of expression are not compatible with the operation\n");
  } else {
    type_checking_num_errors++; printf("ERROR: operands of expression are not compatible with the operation\n");
  }
}

bool types_are_compatible(struct type *left, /* in */
                          struct type *right /* in */) {
    bool compatible = false;
    printf("Type of left: %d\n", left->kind);
    printf("Type of right: %d\n", right->kind);
    if(type_is_pointer(left) && type_is_pointer(right)) {
        compatible = types_are_compatible(left->data.pointer.pointee, right->data.pointer.pointee);
    } else if (type_is_arithmetic(left) && type_is_arithmetic(right)) {
        compatible = true;
    } else if (type_is_array(left) && type_is_pointer(right)) {
        compatible = types_are_compatible(left->data.array.array_type, right->data.pointer.pointee);
    } else if (type_is_pointer(left) && type_is_array(right)) {
        compatible = types_are_compatible(left->data.pointer.pointee, right->data.array.array_type);
    }
    return compatible;
}

void type_convert_simple_assignment(struct node *binary_operation) {
  struct type *left_operand_type = node_get_result(binary_operation->data.binary_operation.left_operand)->type;
  struct type *right_operand_type = node_get_result(binary_operation->data.binary_operation.right_operand)->type;
  struct type *result_type;
  assert(NODE_BINARY_OPERATION == binary_operation->kind);
  if(!node_is_lvalue(binary_operation->data.binary_operation.left_operand)) {
      type_checking_num_errors++;
      printf("ERROR: The left operand of an assignment operation must be an lvalue\n");
      return;
  }
  if(type_is_arithmetic(left_operand_type) && type_is_arithmetic(right_operand_type)) {
      result_type = left_operand_type;
      /* Cast the right operand to the type of the left operand */
      add_cast_expr(binary_operation->data.binary_operation.right_operand, result_type);
  } else if(type_is_pointer(left_operand_type) && type_is_pointer(right_operand_type)) {
      if(types_are_compatible(left_operand_type, right_operand_type)) {
          result_type = left_operand_type;
          add_cast_expr(binary_operation->data.binary_operation.right_operand, result_type);
      } else {
       type_checking_num_errors++;
       printf("ERROR: the pointer types in the assignment operation are not compatible\n");
      }
  } else if(type_is_pointer(left_operand_type) && type_is_arithmetic(right_operand_type)) {
      if((binary_operation->data.binary_operation.right_operand->kind == NODE_NUMBER) &&
         (binary_operation->data.binary_operation.right_operand->data.number.value == 0)) {
          result_type = left_operand_type;
          add_cast_expr(binary_operation->data.binary_operation.right_operand, result_type);
      } else {
          type_checking_num_errors++;
          printf("ERROR: The left operand is pointer and the right operand is a number which is not 0. This is not allowed\n");
      }
  } /* xxx add void type checking too.. */
  else {
      type_checking_num_errors++;
      printf("ERROR: operands of assignment operation are not compatible\n");
  }

    type_convert_assignment(binary_operation);
}

void type_convert_compound_assignment(struct node *binary_operation) {
  struct type *left_operand_type = node_get_result(binary_operation->data.binary_operation.left_operand)->type;
  struct type *right_operand_type = node_get_result(binary_operation->data.binary_operation.right_operand)->type;
  struct type *result_type;
  assert(NODE_BINARY_OPERATION == binary_operation->kind);
  if(!node_is_lvalue(binary_operation->data.binary_operation.left_operand)) {
      type_checking_num_errors++;
      printf("ERROR: The left operand of an assignment operation must be an lvalue\n");
      return;
  }
  if(type_is_arithmetic(left_operand_type) && type_is_arithmetic(right_operand_type)) {
      result_type = left_operand_type;
      /* Cast the right operand to the type of the left operand */
      add_cast_expr(binary_operation->data.binary_operation.right_operand, result_type);
  }  else {
      type_checking_num_errors++;
      printf("ERROR: operands of assignment operation are not compatible\n");
  }

    type_convert_assignment(binary_operation);
}

/* The left operand for += and -= can be pointer or integer. Taking care of this
 * case */
void type_convert_scalar_compound_assignment(struct node *binary_operation) {
  struct type *left_operand_type = node_get_result(binary_operation->data.binary_operation.left_operand)->type;
  struct type *right_operand_type = node_get_result(binary_operation->data.binary_operation.right_operand)->type;
  struct type *result_type;
  assert(NODE_BINARY_OPERATION == binary_operation->kind);
  if(!node_is_lvalue(binary_operation->data.binary_operation.left_operand)) {
      type_checking_num_errors++;
      printf("ERROR: The left operand of an assignment operation must be an lvalue\n");
      return;
  }
  if(type_is_scalar(left_operand_type) && type_is_arithmetic(right_operand_type)) {
      result_type = left_operand_type;
      /* Cast the right operand to the type of the left operand */
      add_cast_expr(binary_operation->data.binary_operation.right_operand, result_type);
  }  else {
      type_checking_num_errors++;
      printf("ERROR: operands of assignment operation are not compatible\n");
  }

    type_convert_assignment(binary_operation);
}

void type_assign_in_ternary_operation(struct node *ternary_operation) {
  assert(NODE_TERNARY_OPERATION == ternary_operation->kind);
  type_assign_in_expression(ternary_operation->data.ternary_operation.first_operand);
  type_assign_in_expression(ternary_operation->data.ternary_operation.second_operand);
  type_assign_in_expression(ternary_operation->data.ternary_operation.third_operand);
  /* xxx: Unsure whether we need to do any casting conversions here */
}

void type_assign_in_binary_operation(struct node *binary_operation) {
  assert(NODE_BINARY_OPERATION == binary_operation->kind);
  type_assign_in_expression(binary_operation->data.binary_operation.left_operand);
  type_assign_in_expression(binary_operation->data.binary_operation.right_operand);

  switch (binary_operation->data.binary_operation.operation) {
    case BINOP_MULTIPLICATION:
    case BINOP_DIVISION:
    case BINOP_REMAINDER:
      type_convert_arithmetic(binary_operation);
      break;
    case BINOP_ADDITION:
    case BINOP_SUBTRACTION:
      type_convert_additive(binary_operation);
      break;
    case BINOP_ASSIGN:
      type_convert_simple_assignment(binary_operation);
      break;
    case BINOP_ASSIGN_PLUS_EQUAL:
    case BINOP_ASSIGN_MINUS_EQUAL:
      type_convert_scalar_compound_assignment(binary_operation);
      break;
    case BINOP_ASSIGN_ASTERISK_EQUAL:
    case BINOP_ASSIGN_SLASH_EQUAL:
    case BINOP_ASSIGN_PERCENT_EQUAL:
    case BINOP_ASSIGN_LESS_LESS_EQUAL:
    case BINOP_ASSIGN_GREATER_GREATER_EQUAL:
    case BINOP_ASSIGN_AMPERSAND_EQUAL:
    case BINOP_ASSIGN_CARET_EQUAL:
    case BINOP_ASSIGN_VBAR_EQUAL:
      type_convert_compound_assignment(binary_operation);
      /* xxx: Compound assignment */
      break;
    case BINOP_LOGICAL_OR_EXPR:
    case BINOP_LOGICAL_AND_EXPR:
    case BINOP_BITWISE_OR_EXPR:
      type_convert_scalar(binary_operation);
      break;
    case BINOP_BITWISE_XOR_EXPR:
    case BINOP_BITWISE_AND_EXPR:
      type_convert_arithmetic(binary_operation);
      break;

      /* Different kinds of equality operators
       */
    case BINOP_IS_EQUAL_TO:
    case BINOP_NOT_EQUAL_TO:

      /* Different relational, shift, additive
       * and multiplicative operators
       */
    case BINOP_LESS_THAN:
    case BINOP_LESS_THAN_OR_EQUAL_TO:
    case BINOP_GREATER_THAN:
    case BINOP_GREATER_THAN_OR_EQUAL_TO:
      type_convert_scalar(binary_operation);
      break;
    case BINOP_SHIFT_LEFT:
    case BINOP_SHIFT_RIGHT:
      type_convert_arithmetic(binary_operation);
      break;
    default:
      assert(0);
      break;
  }
}

/* Set the type to be that of the cast */
void type_assign_in_cast_expr(struct node *cast_expr) {
    cast_expr->data.cast_expr.result.type =
        get_type_from_type_specifier(
            cast_expr->data.cast_expr.unary_casting_expr->data.unary_operation.the_operand);
}

void type_assign_in_expr(struct node *expr) {
    assert(NODE_EXPR == expr->kind);
    if(expr->data.expr.expr1 != NULL) {
        type_assign_in_expression(expr->data.expr.expr1);
    }
    if(expr->data.expr.expr2 != NULL) {
        type_assign_in_expression(expr->data.expr.expr2);
    }
}

void type_assign_in_expression_list(struct node *expression_list,
				    int * number_of_parameters,
                                    struct symbol_list *parameter_list) {
  (*number_of_parameters)++;
  if(expression_list->data.expression_list.assignment_expr != NULL) {
      type_assign_in_expression(expression_list->data.expression_list.assignment_expr);
      printf("Type of prototype argument: %d\n", parameter_list->symbol.result.type->kind);
      printf("Type of function call argument: %d\n", node_get_result(expression_list->data.expression_list.assignment_expr)->type->kind);
      if(!types_are_compatible(parameter_list->symbol.result.type,
                               node_get_result(expression_list->data.expression_list.assignment_expr)->type)) {
          type_checking_num_errors++;
          printf("ERROR: The type of arguments passed in to the function do not match the prototype\n");
      } else if(((parameter_list->next != NULL) &&
                 (expression_list->data.expression_list.expression_list == NULL)) ||
                ((parameter_list->next == NULL) &&
                 (expression_list->data.expression_list.expression_list != NULL))) {
          type_checking_num_errors++;
          printf("ERROR: Number of parameters in function call not same as declaration or definition\n");
      } else {
          parameter_list = parameter_list->next;
          if(expression_list->data.expression_list.expression_list != NULL) {
              type_assign_in_expression_list(expression_list->data.expression_list.expression_list,
                                             number_of_parameters, parameter_list);
          }
      }
  }
}

void type_assign_in_function_call(struct node *function_call) {
  struct type *postfix_expr_type;
  struct type *return_type_of_function;
  int number_of_parameters = 0;
  struct symbol_list *parameter_list;
  /* struct type *parameter_type; */
    assert(NODE_FUNCTION_CALL == function_call->kind);
    if(function_call->data.function_call.postfix_expr != NULL) {
      type_assign_in_expression(function_call->data.function_call.postfix_expr);
    }
    postfix_expr_type = node_get_result(function_call->data.function_call.postfix_expr)->type;
    assert(postfix_expr_type->kind == TYPE_FUNCTION);
    return_type_of_function = postfix_expr_type->data.function.return_type;
    printf("Return type of postfix expr: %d\n", return_type_of_function->kind);
    printf("Number of parameters: %d\n", postfix_expr_type->data.function.number_of_parameters);
    parameter_list = postfix_expr_type->data.function.parameter_list;
    if(function_call->data.function_call.expression_list != NULL) {
        type_assign_in_expression_list(function_call->data.function_call.expression_list,
                                       &number_of_parameters, parameter_list);

    }
    function_call->data.function_call.result.type = return_type_of_function;
}

void type_assign_in_subscript_expr(struct node *subscript_expr) {
  struct type *type;
  assert(NODE_SUBSCRIPT_EXPR == subscript_expr->kind);
  if(subscript_expr->data.subscript_expr.postfix_expr != NULL) {
    type_assign_in_expression(subscript_expr->data.subscript_expr.postfix_expr);
  }
  type = node_get_result(subscript_expr->data.subscript_expr.postfix_expr)->type;
  assert(type->kind == TYPE_ARRAY);
  node_get_result(subscript_expr)->type = type->data.array.array_type;
}

void type_assign_in_statement(struct node *statement) {
  printf("Statement kind: %d\n", statement->kind);
  assert(NODE_STATEMENT == statement->kind);
  if (NULL != statement->data.statement.statement) {
    type_assign_in_statement(statement->data.statement.statement);
  }
  if(statement->data.statement.type_of_statement== RETURN_STATEMENT_TYPE) {
      printf("Return statement!\n");
  }
  type_assign_in_expression(statement->data.statement.expression);
}

void type_assign_in_statement_list(struct node *statement_list) {
  assert(NODE_STATEMENT_LIST == statement_list->kind);
  if (NULL != statement_list->data.statement_list.init) {
    type_assign_in_expression(statement_list->data.statement_list.init);
  }
  type_assign_in_expression(statement_list->data.statement_list.statement);
}

void type_assign_in_compound_statement(struct node *compound_statement) {
    if(compound_statement->data.compound_statement.declaration_or_statement_list != NULL) {
        type_assign_in_expression(compound_statement->data.compound_statement.declaration_or_statement_list);
    }
}

void type_assign_in_function_def_specifier(struct node *function_def_specifier) {
    /* struct type *return_type = */
    /*     get_type_from_type_specifier( */
    /*         function_def_specifier->data.function_def_specifier.decl_specifier); */
    type_assign_in_expression(function_def_specifier->data.function_def_specifier.declarator);

}

void type_assign_in_function_definition(struct node *function_definition) {
    type_assign_in_expression(function_definition->data.function_definition.function_def_specifier);
    type_assign_in_expression(function_definition->data.function_definition.compound_statement);
}

void type_assign_in_expression(struct node *expression) {
  switch (expression->kind) {
    case NODE_UNARY_OPERATION:
      type_assign_in_unary_operation(expression);
      break;

    case NODE_IDENTIFIER:
      printf("Name : %s\n", expression->data.identifier.symbol->name);
      if (NULL == expression->data.identifier.symbol->result.type) {
          printf("ERROR: The identifier's type should have been defined by now\n");
          assert(0);
      }
      break;

    case NODE_NUMBER:
      expression->data.number.result.type =
          type_basic(false, TYPE_WIDTH_INT, CONVERSION_RANK_INT);
      break;

    case NODE_BINARY_OPERATION:
      type_assign_in_binary_operation(expression);
      break;
    case NODE_STRING:
      expression->data.string.result.type =
          type_pointer(type_basic(false, TYPE_WIDTH_CHAR, CONVERSION_RANK_CHAR));
      break;
    case NODE_TERNARY_OPERATION:
      /* xxx: type_assign_in_ternary_operation(expression); */
      break;
    case NODE_STATEMENT:
      type_assign_in_statement(expression);
      break;
    case NODE_STATEMENT_LIST:
      type_assign_in_statement_list(expression);
      break;
    case NODE_DECL:
      /* Nothing to do */
      break;
    case NODE_TYPE_SPECIFIER:
      /* Nothing to do */
      break;
    case NODE_POINTER:
      break;
    case NODE_EXPR:
      type_assign_in_expr(expression);
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
      type_assign_in_function_def_specifier(expression);
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
      type_assign_in_compound_statement(expression);
      break;
    case NODE_FUNCTION_DEFINITION:
      type_assign_in_function_definition(expression);
      break;
    case NODE_CAST_EXPR:
      type_assign_in_cast_expr(expression);
      break;
    case NODE_FUNCTION_CALL:
      type_assign_in_function_call(expression);
      break;
    case NODE_SUBSCRIPT_EXPR:
      type_assign_in_subscript_expr(expression);
      break;
    default:
      assert(0);
      break;
  }
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
