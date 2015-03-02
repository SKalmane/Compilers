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

struct node *node_unary_operation(int operation,
                                  struct node *the_operand)
{
  struct node *node = node_create(NODE_UNARY_OPERATION);
  node->data.unary_operation.operation = operation;
  node->data.unary_operation.the_operand = the_operand;
  node->data.unary_operation.result.type = NULL;
  node->data.unary_operation.result.ir_operand = NULL;
  return node;
}

struct node *node_binary_operation(struct node *left_operand,
                                   int operation,
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

/* The only ternary operation we support is expr ? expr : expr */

struct node *node_ternary_operation(struct node *first_operand,
                                    struct node *second_operand,
                                    struct node *third_operand)
{
  struct node *node = node_create(NODE_TERNARY_OPERATION);
  node->data.ternary_operation.first_operand = first_operand;
  node->data.ternary_operation.second_operand = second_operand;
  node->data.ternary_operation.third_operand = third_operand;
  node->data.ternary_operation.result.type = NULL;
  node->data.ternary_operation.result.ir_operand = NULL;
  return node;
}

struct node *node_statement(struct node *statement,
                            struct node *expression,
                            int type_of_statement)
{
  struct node *node = node_create(NODE_STATEMENT);
  node->data.statement.statement = statement;
  node->data.statement.expression = expression;
  node->data.statement.type_of_statement = type_of_statement;
  return node;
}

struct node *node_pointer(struct node *pointer) {
  struct node *node = node_create(NODE_POINTER);
  node->data.pointer.pointer = pointer;
  return node;
}

struct node *node_type_specifier(int kind_of_type_specifier) {
    struct node *node = node_create(NODE_TYPE_SPECIFIER);
    /* printf("Creating type specifier of kind %d\n", kind_of_type_specifier); */
    node->data.type_specifier.kind_of_type_specifier = kind_of_type_specifier;
    return node;
}

struct node *node_statement_list(struct node *init, struct node *statement) {
  struct node *node = node_create(NODE_STATEMENT_LIST);
  node->data.statement_list.init = init;
  node->data.statement_list.statement = statement;
  return node;
}

struct node *node_abstract_decl(struct node *abstract_direct_declarator,
                                struct node *expression,
                                int type_of_abstract_decl) {
    struct node *node = node_create(NODE_ABSTRACT_DECL);
    node->data.abstract_decl.abstract_direct_declarator = abstract_direct_declarator;
    node->data.abstract_decl.expression = expression;
    node->data.abstract_decl.type_of_abstract_decl = type_of_abstract_decl;
    return node;
}

struct node *node_expr(struct node *expr1,
                       struct node *expr2,
                       int type_of_expr) {
    struct node *node = node_create(NODE_EXPR);
    node->data.expr.expr1 = expr1;
    node->data.expr.expr2 = expr2;
    node->data.expr.type_of_expr = type_of_expr;
    return node;
}

struct node *node_for_expr(struct node *initial_clause,
                           struct node *expr1,
                           struct node *expr2) {
    struct node *node = node_create(NODE_FOR_EXPR);
    node->data.for_expr.initial_clause = initial_clause;
    node->data.for_expr.expr1 = expr1;
    node->data.for_expr.expr2 = expr2;
    return node;
}

struct node *node_translation_unit(struct node *translation_unit,
				   struct node *top_level_decl) {
  struct node *node = node_create(NODE_TRANSLATION_UNIT);
  node->data.translation_unit.translation_unit = translation_unit;
  node->data.translation_unit.top_level_decl = top_level_decl;
  return node;
}

struct node *node_if_statement(struct node *expr,
			       struct node *if_statement,
			       struct node *else_statement) {
  struct node *node = node_create(NODE_IF_STATEMENT);
  node->data.if_statement.expr = expr;
  node->data.if_statement.if_statement = if_statement;
  node->data.if_statement.else_statement = else_statement;
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
    "%",    /* BINOP_REMAINDER                                   4 */
    "=",    /*  5 = BINOP_ASSIGN */
    "+=",    /*  6 = BINOP_ASSIGN_PLUS_EQUAL */
    "-=",    /*  7 = BINOP_ASSIGN_MINUS_EQUAL */
    "*=",    /*  8 = BINOP_ASSIGN_ASTERISK_EQUAL */
    "/=",    /*  9 = BINOP_ASSIGN_SLASH_EQUAL */
    "%=",    /*  10 = BINOP_ASSIGN_PERCENT_EQUAL */
    "<<=",   /* BINOP_ASSIGN_LESS_LESS_EQUAL = 11 */
    ">>=",   /* BINOP_ASSIGN_GREATER_GREATER_EQUAL               12 */
    "&=",    /* BINOP_ASSIGN_AMPERSAND_EQUAL                     13 */
    "^=",    /* BINOP_ASSIGN_CARET_EQUAL                         14 */
    "|=",    /* BINOP_ASSIGN_VBAR_EQUAL                          15 */
    "||",    /* BINOP_LOGICAL_OR_EXPR                            16 */
    "&&",    /* BINOP_LOGICAL_AND_EXPR                           17 */
    "|",     /* BINOP_BITWISE_OR_EXPR                            18 */
    "^",     /* BINOP_BITWISE_XOR_EXPR                           19 */
    "&",     /* BINOP_BITWISE_AND_EXPR                           20 */
    "==",    /* BINOP_IS_EQUAL_TO                                21 */
    "!=",    /* BINOP_NOT_EQUAL_TO                               22 */
    "<",     /* BINOP_LESS_THAN                                  23 */
    "<=",    /* BINOP_LESS_THAN_OR_EQUAL_TO                      24 */
    ">",     /* BINOP_GREATER_THAN                               25 */
    ">=",    /* BINOP_GREATER_THAN_OR_EQUAL_TO                   26 */
    "<<",    /* BINOP_SHIFT_LEFT                                 27 */
    ">>",    /* BINOP_SHIFT_RIGHT                                28 */
    ",",     /* BINOP_SEQUENTIAL_EVALUATION                      29 */
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

void node_print_unary_operation(FILE *output, struct node *unary_operation) {
    static const char *unary_operators[] = {
        "++",           /*  UNARYOP_PREFIX_INCREMENT                          0 */
        "--",           /*  UNARYOP_PREFIX_DECREMENT                          1 */
        "sizeof",       /*  UNARYOP_SIZEOF                                    2 */
        "~",            /*  UNARYOP_BITWISE_NOT                               3 */
        "!",            /*  UNARYOP_LOGICAL_NOT                               4 */
        "-",            /*  UNARYOP_NEGATION                                  5 */
        "+",            /*  UNARYOP_PLUS                                      6 */
        "&",            /*  UNARYOP_ADDRESS_OF                                7 */
        "*",            /*  UNARYOP_INDIRECTION                               8 */
        "()",           /*  UNARYOP_CASTING                                   9  xxx */
        "[]",           /*  UNARYOP_SUBSCRIPTING                             10  xxx */
        "()",           /*  UNARYOP_FUNCTION_CALL                            11 xxx */
        ".",            /*  UNARYOP_DIRECT_SELECTION                         13 /\* a.data *\/ */
        "->",           /*  UNARYOP_INDIRECT_SELECTION                       14 /\* a->data *\/ */
        "++",           /*  UNARYOP_POSTFIX_INCREMENT                        15 */
        "--",           /*  UNARYOP_POSTFIX_DECREMENT                        16 */
        NULL
    };

  assert(NULL != unary_operation && NODE_UNARY_OPERATION == unary_operation->kind);

  fputs("(", output);
  fputs(" ", output);
  if(unary_operation->data.unary_operation.operation <= 9) { /* prefix operators */
      fputs(unary_operators[unary_operation->data.unary_operation.operation], output);
  }
  node_print_expression(output, unary_operation->data.unary_operation.the_operand);
  if(unary_operation->data.unary_operation.operation > 9) { /* prefix operators */
      fputs(unary_operators[unary_operation->data.unary_operation.operation], output);
  }
  fputs(" ", output);
  fputs(")", output);
}

void node_print_number(FILE *output, struct node *number) {
  assert(NULL != number);
  assert(NODE_NUMBER == number->kind);

  if(number->data.number.result.type->data.basic.width == TYPE_WIDTH_CHAR) {
      /* Should be a character */
      assert(number->data.number.value <= 255);
      fputs("\'", output);
      fprintf(output, "%c", (int) number->data.number.value);
      fputs("\'", output);
  } else {
      fprintf(output, "%lu", number->data.number.value);
  }
}

void node_print_string(FILE *output, struct node *string) {
    int length = string->data.string.length;

    char *str = string->data.string.name;
    int i;
    fputs("\"", output);
    for (i = 0; i < length; i++) {
        if (str[i] == 0) {
            fputs("\\0", output);
        } else {
            fprintf(output, "%c", str[i]);
        }
    }
    fputs("\"", output);
}

void node_print_ternary_operation(FILE *output, struct node *ternary_operation) {

    fputs("( ", output);
    node_print_expression(output, ternary_operation->data.ternary_operation.first_operand);
    fputs(" )  ? ", output);
    fputs("( ", output);
    node_print_expression(output, ternary_operation->data.ternary_operation.second_operand);
    fputs(") : ", output);
    fputs("( ", output);
    node_print_expression(output, ternary_operation->data.ternary_operation.third_operand);
    fputs(") ", output);
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
  /* fprintf(output, "/\* %p *\/", (void *)identifier->data.identifier.symbol); xxx */
}

void node_print_type_specifier(FILE *output, struct node *identifier) {
  assert(NULL != identifier);
  assert(NODE_TYPE_SPECIFIER == identifier->kind);
  switch (identifier->data.type_specifier.kind_of_type_specifier) {
    case SIGNED_SHORT_INT:
      fprintf(output, "signed short ");
      break;
    case SIGNED_INT:
      fprintf(output, "int ");
      break;
    case SIGNED_LONG_INT:
      fprintf(output, "signed long ");
      break;
    case UNSIGNED_SHORT_INT:
      fprintf(output, "unsigned short ");
      break;
    case UNSIGNED_INT:
      fprintf(output, "unsigned int ");
      break;
    case UNSIGNED_LONG_INT:
      fprintf(output, "unsigned long ");
      break;
    case CHARACTER_TYPE:
      fprintf(output, "char ");
      break;
    case SIGNED_CHARACTER_TYPE:
      fprintf(output, "signed char ");
      break;
    case UNSIGNED_CHARACTER_TYPE:
      fprintf(output, "unsigned char ");
      break;
    case VOID_TYPE:
      fprintf(output, "void ");
      break;
  }
}

void node_print_pointer(FILE *output, struct node *pointer) {
    fputs("*", output);
    if(pointer->data.pointer.pointer != NULL) {
        node_print_expression(output, pointer->data.pointer.pointer);
    }
}

void node_print_abstract_decl(FILE *output, struct node *abstract_declarator) {
    assert(NODE_ABSTRACT_DECL == abstract_declarator->kind);
    switch(abstract_declarator->data.abstract_decl.type_of_abstract_decl) {
      case PARENTHESIZED_ABSTRACT_DECL:
        assert(abstract_declarator->data.abstract_decl.abstract_direct_declarator == NULL);
        fputs("(", output);
        node_print_expression(output, abstract_declarator->data.abstract_decl.expression);
        fputs(")", output);
        break;
      case SQUARE_BRACKETS_ABSTRACT_DECL:
        if(abstract_declarator->data.abstract_decl.abstract_direct_declarator != NULL) {
            node_print_expression(output, abstract_declarator->data.abstract_decl.abstract_direct_declarator);
        }
        fputs("[", output);
        if(abstract_declarator->data.abstract_decl.expression != NULL) {
            node_print_expression(output, abstract_declarator->data.abstract_decl.expression);
        }
        fputs("]", output);
    }
}

void node_print_for_expr(FILE *output, struct node *for_expr) {
    assert(NODE_FOR_EXPR == for_expr->kind);
    fputs("(", output);
    if(for_expr->data.for_expr.initial_clause != NULL) {
        node_print_expression(output, for_expr->data.for_expr.initial_clause);
    }
    fputs("; ", output);
    if(for_expr->data.for_expr.expr1 != NULL) {
        node_print_expression(output, for_expr->data.for_expr.expr1);
    }
    fputs("; ", output);
    if(for_expr->data.for_expr.expr2 != NULL) {
        node_print_expression(output, for_expr->data.for_expr.expr2);
    }
    fprintf(output,") {\n");
}

void node_print_statement(FILE *output, struct node *statement) {
  assert(NULL != statement);
  assert(NODE_STATEMENT == statement->kind);

  switch (statement->data.statement.type_of_statement) {
    case COMPOUND_STATEMENT_TYPE:
      fputs("{\n", output);
      if(statement->data.statement.statement != NULL) {
	node_print_expression(output, statement->data.statement.statement);
      }
      fputs("\n}\n\n", output);
      break;
    case EXPRESSION_STATEMENT_TYPE:
      node_print_expression(output, statement->data.statement.expression);      
      fputs(";\n", output);
      break;
    case LABELED_STATEMENT_TYPE:
      node_print_expression(output, statement->data.statement.expression);      
      fputs(" : ", output);
      node_print_expression(output, statement->data.statement.statement);      
      break;
    case WHILE_STATEMENT_TYPE:
      fprintf(output, "while(");
      node_print_expression(output, statement->data.statement.expression);      
      fputs(")",output);
      node_print_expression(output, statement->data.statement.statement);
      break;
    case DO_STATEMENT_TYPE:
      fprintf(output, "do");
      node_print_expression(output, statement->data.statement.statement);
      fprintf(output, "while(");      
      node_print_expression(output, statement->data.statement.expression);      
      fputs(")",output);
      break;      
    case FOR_STATEMENT_TYPE:
      fprintf(output, "for");
      node_print_expression(output, statement->data.statement.expression);
      node_print_expression(output, statement->data.statement.statement);      
      break;      
    case BREAK_STATEMENT_TYPE:
      fprintf(output, "break;\n");
      break;   
    case CONTINUE_STATEMENT_TYPE:
      fprintf(output, "continue;\n");
      break;   
    case RETURN_STATEMENT_TYPE:
      fprintf(output, "return ");
      node_print_expression(output, statement->data.statement.expression);
      fputs(";\n", output);
      break;   
    case GOTO_STATEMENT_TYPE:
      fprintf(output, "goto ");
      node_print_expression(output, statement->data.statement.expression);
      fputs(";\n", output);
      break;  
    case NULL_STATEMENT_TYPE:
      fprintf(output, ";\n");
      break;
    default:
      assert(0);
      break;
  }

}

void node_print_expr(FILE *output, struct node *expr) {
  assert(NODE_EXPR == expr->kind);

  switch (expr->data.expr.type_of_expr) {
    case BASE_EXPR:
      assert(expr->data.expr.expr1 == NULL);
      node_print_expression(output, expr->data.expr.expr2);
      break;
    case SUBSCRIPT_EXPR:
      node_print_expression(output, expr->data.expr.expr1);
      fputs("[", output);
      if(NULL != expr->data.expr.expr2) {
          node_print_expression(output, expr->data.expr.expr2);
      }
      fputs("]", output);
      break;
    case EXPRESSION_LIST:
      node_print_expression(output, expr->data.expr.expr1);
      fputs(", ", output);
      node_print_expression(output, expr->data.expr.expr2);
      break;
    case FUNCTION_CALL:
      node_print_expression(output, expr->data.expr.expr1);
      fputs("(", output);
      if(NULL != expr->data.expr.expr2) {
          node_print_expression(output, expr->data.expr.expr2);
      }
      fputs(") ", output);
      break;
    case CONCAT_EXPR:
      node_print_expression(output, expr->data.expr.expr1);
      fputs(" ", output);
      node_print_expression(output, expr->data.expr.expr2);
      break;
    case DECL_STATEMENT:
      node_print_expression(output, expr->data.expr.expr1);
      fputs("( ", output);
      node_print_expression(output, expr->data.expr.expr2);
      fputs(" );\n", output);
      break;
    case COMMA_SEPARATED_STATEMENT:
      node_print_expression(output, expr->data.expr.expr1);
      fputs(", ", output);
      node_print_expression(output, expr->data.expr.expr2);
      break;
    default:
      fprintf(output, "Expr not found!\n");
      assert(0);
      break;
  }

  /* fputs(";\n", output); xxx */
}

void node_print_if_statement(FILE *output, struct node *statement) {
  assert(NODE_IF_STATEMENT == statement->kind);
  fprintf(output, "if(");
  node_print_expression(output, statement->data.if_statement.expr);
  fprintf(output, ") {\n");
  node_print_expression(output, statement->data.if_statement.if_statement);
  fprintf(output, "} ");
  if(statement->data.if_statement.else_statement != NULL) {
    fprintf(output, "else { \n");
    node_print_expression(output, statement->data.if_statement.else_statement);
    fprintf(output, "} ");
  }
  fprintf(output, "\n");
}

void node_print_expression(FILE *output, struct node *expression) {
  assert(NULL != expression);
  switch (expression->kind) {
    case NODE_UNARY_OPERATION:
      node_print_unary_operation(output, expression);
      break;
    case NODE_BINARY_OPERATION:
      node_print_binary_operation(output, expression);
      break;
    case NODE_TERNARY_OPERATION:
      node_print_ternary_operation(output, expression);
      break;
    case NODE_STATEMENT:
      node_print_statement(output, expression);
      break;
    case NODE_STATEMENT_LIST:
      node_print_statement_list(output, expression);
      break;
    case NODE_IDENTIFIER:
      node_print_identifier(output, expression);
      break;
    case NODE_NUMBER:
      node_print_number(output, expression);
      break;
    case NODE_STRING:
      node_print_string(output, expression);
      break;
    case NODE_EXPR:
      node_print_expr(output, expression);
      break;
    case NODE_TYPE_SPECIFIER:
      node_print_type_specifier(output, expression);
      break;
    case NODE_POINTER:
      node_print_pointer(output, expression);
      break;
    case NODE_ABSTRACT_DECL:
      node_print_abstract_decl(output, expression);
      break;
    case NODE_FOR_EXPR:
      node_print_for_expr(output, expression);
      break;
    case NODE_IF_STATEMENT:
      node_print_if_statement(output, expression);
      break;
    default:
      fprintf(output, "Type of expression is %d\n", expression->kind);
      fprintf(output, "Can't recognize expression!\n");
      assert(0);
      break;
  }
}

void node_print_statement_list(FILE *output, struct node *statement_list) {
  assert(NODE_STATEMENT_LIST == statement_list->kind);

  if (NULL != statement_list->data.statement_list.init) {
    node_print_expression(output, statement_list->data.statement_list.init);
  }
  node_print_expression(output, statement_list->data.statement_list.statement);
  fputs("\n", output);
}

void node_print_translation_unit(FILE *output, struct node *translation_unit) {
  assert(NODE_TRANSLATION_UNIT == translation_unit->kind);
  if(translation_unit->data.translation_unit.translation_unit != NULL) {
    assert(NODE_TRANSLATION_UNIT == translation_unit->data.translation_unit.translation_unit->kind);
    node_print_translation_unit(output, translation_unit->data.translation_unit.translation_unit);
  }
  if(translation_unit->data.translation_unit.top_level_decl != NULL) {
    node_print_expression(output, translation_unit->data.translation_unit.top_level_decl);
  }
}
