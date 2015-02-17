#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>

#include "node.h"
#include "symbol.h"
#include "type.h"

extern int yylineno;

/****************
 * CREATE NODES *
 ****************/

/* Allocate and initialize a generic node. */
struct node *node_create(int node_kind) {
  struct node *n;

  n = malloc(sizeof(struct node));
  assert(NULL != n);

  n->kind = node_kind;
  n->line_number = yylineno;
  n->ir = NULL;
  return n;
}

/* Allocate and initialize a generic type. */
struct type *type_create(int type_kind) {
  struct type *n;

  n = malloc(sizeof(struct type));
  assert(NULL != n);

  n->kind = type_kind;
  return n;
}

/*
 * node_identifier - allocate a node to represent an identifier
 *
 * Parameters:
 *   text - string - contains the name of the identifier
 *   length - integer - the length of text (not including terminating NUL)
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_identifier(char *text, int length)
{
  struct node *node = node_create(NODE_IDENTIFIER);
  memset(node->data.identifier.name, 0, MAX_IDENTIFIER_LENGTH + 1);
  strncpy(node->data.identifier.name, text, length);
  node->data.identifier.symbol = NULL;
  return node;
}


/*
 * node_string - allocate a node to represent a string
 *
 * Parameters:
 *   text - string - contains the entire string
 *   length - integer - the length of text (not including terminating NUL)
 *
 * Returns a NUL-terminated string in newly allocated memory, containing the
 *   string. Returns NULL if memory could not be allocated.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_string(char *text, int length)
{
  int i;
  struct node *node = node_create(NODE_STRING);
  memset(node->data.string.name, 0, MAX_STRING_LENGTH + 1);
  if(length > MAX_STRING_LENGTH) {
    fprintf(stderr, "String length is greater than Maximum length allowed.. \n");
    /* If string length > Max String length, only display the string
     * till the Max string length
     */
    length = MAX_STRING_LENGTH;
  }
  node->data.string.length = length;
  /* The following loop is to ensure we copy over the
   * \0 character properly
   */
  for(i = 0; i < length; i++) {
    node->data.string.name[i] = text[i];
  }
  /* Wrap up the string with the NULL character at the end */
  node->data.string.name[length] = 0;
  return node;
}


/*
 * node_number - allocate a node to represent a number
 *
 * Parameters:
 *   text - string - contains the numeric literal
 *   length - integer - the length of text (not including terminating NUL)
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 */
struct node *node_number(char *text)
{
  struct node *node = node_create(NODE_NUMBER);

  errno = 0;
  node->data.number.value = strtoul(text, NULL, 10);
  node->data.number.overflow = false;
  node->data.number.result.type = type_create(TYPE_BASIC);

  if (node->data.number.value == ULONG_MAX && ERANGE == errno) {
    /* Strtoul indicated overflow. */
    node->data.number.overflow = true;
  } else if (node->data.number.value > 4294967295ul) {
    /* Value is too large for 32-bit unsigned long type. */
    node->data.number.overflow = true;
  } else if (node->data.number.value < 2147483648ul) {
    struct type *type = node->data.number.result.type;
    type->data.basic.width = TYPE_WIDTH_INT;
    type->data.basic.is_unsigned = false;
  } else {
    struct type *type = node->data.number.result.type;
    type->data.basic.width = TYPE_WIDTH_LONG;
    type->data.basic.is_unsigned = true;
  }

  node->data.number.result.ir_operand = NULL;
  return node;
}

/*
 * node_character - allocate a node to represent a character
 *
 * Parameters:
 *   text - char - the char value which we will store as an int
 *
 * Returns a node containing the value and an error flag. The value is computed by casting the char passed in into int.
 */
struct node *node_character(char text)
{
  struct node *node = node_create(NODE_NUMBER);

  errno = 0;
  /* We know that the text being passed in is a single character */
  node->data.number.value = (int)text;
  node->data.number.result.type = type_create(TYPE_BASIC);
  /* if(text < 0) { */
  /*   /\* Take 2's complement of the number *\/ */
  /*   node->data.number.value = 256 + text;  */
  /* } */
  /* The number is less than 256 since text is a 1 byte character */
  node->data.number.overflow = false;

  node->data.number.result.type->data.basic.width = TYPE_WIDTH_CHAR;
  node->data.number.result.type->data.basic.is_unsigned = false;
  node->data.number.result.ir_operand = NULL;
  return node;
}

struct node *node_binary_operation(int operation, struct node *left_operand,
                                   struct node *right_operand)
{
  struct node *node = node_create(NODE_BINARY_OPERATION);
  node->data.binary_operation.operation = operation;
  node->data.binary_operation.left_operand = left_operand;
  node->data.binary_operation.right_operand = right_operand;
  node->data.binary_operation.result.type = NULL;
  node->data.binary_operation.result.ir_operand = NULL;
  return node;
}

struct node *node_expression_statement(struct node *expression)
{
  struct node *node = node_create(NODE_EXPRESSION_STATEMENT);
  node->data.expression_statement.expression = expression;
  return node;
}

struct node *node_statement_list(struct node *init, struct node *statement) {
  struct node *node = node_create(NODE_STATEMENT_LIST);
  node->data.statement_list.init = init;
  node->data.statement_list.statement = statement;
  return node;
}

struct result *node_get_result(struct node *expression) {
  switch (expression->kind) {
    case NODE_NUMBER:
      return &expression->data.number.result;
    case NODE_IDENTIFIER:
      return &expression->data.identifier.symbol->result;
    case NODE_BINARY_OPERATION:
      return &expression->data.binary_operation.result;
    default:
      assert(0);
      return NULL;
  }
}

/***************************************
 * PARSE TREE PRETTY PRINTER FUNCTIONS *
 ***************************************/

void node_print_expression(FILE *output, struct node *expression);

void node_print_binary_operation(FILE *output, struct node *binary_operation) {
  static const char *binary_operators[] = {
    "*",    /*  0 = BINOP_MULTIPLICATION */
    "/",    /*  1 = BINOP_DIVISION */
    "+",    /*  2 = BINOP_ADDITION */
    "-",    /*  3 = BINOP_SUBTRACTION */
    "=",    /*  4 = BINOP_ASSIGN */
    NULL
  };

  assert(NULL != binary_operation && NODE_BINARY_OPERATION == binary_operation->kind);

  fputs("(", output);
  node_print_expression(output, binary_operation->data.binary_operation.left_operand);
  fputs(" ", output);
  fputs(binary_operators[binary_operation->data.binary_operation.operation], output);
  fputs(" ", output);
  node_print_expression(output, binary_operation->data.binary_operation.right_operand);
  fputs(")", output);
}

void node_print_number(FILE *output, struct node *number) {
  assert(NULL != number);
  assert(NODE_NUMBER == number->kind);

  fprintf(output, "%lu", number->data.number.value);
}

/*
 * After the symbol table pass, we can print out the symbol address
 * for each identifier, so that we can compare instances of the same
 * variable and ensure that they have the same symbol.
 */
void node_print_identifier(FILE *output, struct node *identifier) {
  assert(NULL != identifier);
  assert(NODE_IDENTIFIER == identifier->kind);
  fputs(identifier->data.identifier.name, output);
  fprintf(output, "$%lx", (unsigned long)identifier->data.identifier.symbol);
}

void node_print_expression(FILE *output, struct node *expression) {
  assert(NULL != expression);
  switch (expression->kind) {
    case NODE_BINARY_OPERATION:
      node_print_binary_operation(output, expression);
      break;
    case NODE_IDENTIFIER:
      node_print_identifier(output, expression);
      break;
    case NODE_NUMBER:
      node_print_number(output, expression);
      break;
    default:
      assert(0);
      break;
  }
}

void node_print_expression_statement(FILE *output, struct node *expression_statement) {
  assert(NULL != expression_statement);
  assert(NODE_EXPRESSION_STATEMENT == expression_statement->kind);

  node_print_expression(output, expression_statement->data.expression_statement.expression);

}

void node_print_statement_list(FILE *output, struct node *statement_list) {
  assert(NODE_STATEMENT_LIST == statement_list->kind);

  if (NULL != statement_list->data.statement_list.init) {
    node_print_statement_list(output, statement_list->data.statement_list.init);
  }
  node_print_expression_statement(output, statement_list->data.statement_list.statement);
  fputs(";\n", output);
}
