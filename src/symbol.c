#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "node.h"
#include "symbol.h"
#include "type.h"

int symbol_table_num_errors;

void symbol_initialize_table(struct symbol_table *table,
    int type_of_symbol_table) {
  table->variables = NULL;
  table->type_of_symbol_table = type_of_symbol_table;
  table->statement_labels = NULL;
  table->parent_symbol_table = NULL;
}

/**********************************************
 * WALK PARSE TREE AND ADD SYMBOLS INTO TABLE *
 **********************************************/
/*
 * This function is used to retrieve a symbol from a table.
 */
struct symbol *symbol_get(struct symbol_table *table, char name[]) {
  struct symbol_table *present_symbol_table = table;
  struct symbol_list *iter;
  while(present_symbol_table != NULL) {
      for (iter = present_symbol_table->variables; NULL != iter; iter = iter->next) {
          printf("Present symbol name: %s\n", iter->symbol.name);
          if (!strcmp(name, iter->symbol.name)) {
              return &iter->symbol;
          }
      }
      present_symbol_table = present_symbol_table->parent_symbol_table;
  }
  return NULL;
}

struct symbol *symbol_put(struct symbol_table **table, char name[],
                          struct type *type) {
  struct symbol_list *symbol_list;
  symbol_list = malloc(sizeof(struct symbol_list));
  assert(NULL != symbol_list);
  printf("Adding symbol of name: %s\n", name);
  strncpy(symbol_list->symbol.name, name, MAX_IDENTIFIER_LENGTH);
  symbol_list->symbol.result.type = type;
  symbol_list->symbol.result.ir_operand = NULL;

  symbol_list->next = (*table)->variables;
  (*table)->variables = symbol_list;

  return &symbol_list->symbol;
}

struct symbol *symbol_get_labels(struct symbol_table *table, char name[]) {
  struct symbol_list *iter;
  for (iter = table->statement_labels; NULL != iter; iter = iter->next) {
    if (!strcmp(name, iter->symbol.name)) {
      return &iter->symbol;
    }
  }
  return NULL;
}

struct symbol *symbol_put_labels(struct symbol_table *table, char name[],
                          struct type *type) {
  struct symbol_list *symbol_list;

  symbol_list = malloc(sizeof(struct symbol_list));
  assert(NULL != symbol_list);

  strncpy(symbol_list->symbol.name, name, MAX_IDENTIFIER_LENGTH);
  symbol_list->symbol.result.type = type;
  symbol_list->symbol.result.ir_operand = NULL;

  symbol_list->next = table->statement_labels;
  table->statement_labels = symbol_list;

  return &symbol_list->symbol;
}

void symbol_add_to_function_parameter_list(struct type *function_type,
                                           struct type *param_type) {
    struct symbol_list *symbol_list = malloc(sizeof(struct symbol_list));
    assert(NULL != symbol_list);
    symbol_list->symbol.result.type = param_type;
    symbol_list->symbol.result.ir_operand = NULL;
    symbol_list->next = function_type->data.function.parameter_list;
    function_type->data.function.parameter_list = symbol_list;

    function_type->data.function.number_of_parameters++;
}

void symbol_add_from_identifier(struct symbol_table **table, struct node *identifier,
                                struct type **type) {
  struct symbol *symbol;
  assert(NODE_IDENTIFIER == identifier->kind);
  printf("Identifier name: %s\n", identifier->data.identifier.name);
  symbol = symbol_get(*table, identifier->data.identifier.name);
  if (NULL == symbol) {
      if(type == NULL) {
          symbol_table_num_errors++;
          printf("ERROR: Type of identifier %s not defined\n", identifier->data.identifier.name);
      } else {
          symbol = symbol_put(table, identifier->data.identifier.name, *type);
          printf("Just added to symbol table : %p\n", (void *)(*table));
      }
  }
  identifier->data.identifier.symbol = symbol;
}

void symbol_add_from_identifier_to_labels_list(struct symbol_table *table, struct node *identifier,
                                               struct type *type) {
  struct symbol *symbol;
  assert(NODE_IDENTIFIER == identifier->kind);

  symbol = symbol_get_labels(table, identifier->data.identifier.name);
  if (NULL == symbol) {
      symbol = symbol_put_labels(table, identifier->data.identifier.name, type);
  }
  identifier->data.identifier.symbol = symbol;
}

void symbol_add_from_expression(struct symbol_table *table, struct node *expression,
                                struct type **type);

void symbol_add_from_unary_operation(struct symbol_table *table, struct node *unary_operation) {
  assert(NODE_UNARY_OPERATION == unary_operation->kind);
  symbol_add_from_expression(table, unary_operation->data.unary_operation.the_operand, NULL);
}

void symbol_add_from_binary_operation(struct symbol_table *table, struct node *binary_operation) {
  assert(NODE_BINARY_OPERATION == binary_operation->kind);

  symbol_add_from_expression(table, binary_operation->data.binary_operation.left_operand, NULL);
  symbol_add_from_expression(table, binary_operation->data.binary_operation.right_operand, NULL);
}

void symbol_add_from_ternary_operation(struct symbol_table *table, struct node *ternary_operation) {
  assert(NODE_TERNARY_OPERATION == ternary_operation->kind);

  symbol_add_from_expression(table, ternary_operation->data.ternary_operation.first_operand, NULL);
  symbol_add_from_expression(table, ternary_operation->data.ternary_operation.second_operand, NULL);
  symbol_add_from_expression(table, ternary_operation->data.ternary_operation.third_operand, NULL);
}

void symbol_add_from_statement(struct symbol_table *table, struct node *statement) {
  assert(NODE_STATEMENT == statement->kind);

    if(statement->data.statement.statement != NULL) {
        symbol_add_from_expression(table, statement->data.statement.statement, NULL);
    }
    if(statement->data.statement.expression != NULL) {
        symbol_add_from_expression(table, statement->data.statement.expression, NULL);
    }
}

void symbol_add_from_statement_list(struct symbol_table *table, struct node *statement_list) {
  assert(NODE_STATEMENT_LIST == statement_list->kind);

  if (NULL != statement_list->data.statement_list.init) {
      symbol_add_from_expression(table, statement_list->data.statement_list.init, NULL);
  }
  symbol_add_from_expression(table, statement_list->data.statement_list.statement, NULL);
}

void symbol_add_from_expr(struct symbol_table *table, struct node *expr) {
  assert(NODE_EXPR == expr->kind);
  if(expr->data.expr.expr1 != NULL) {
      symbol_add_from_expression(table, expr->data.expr.expr1, NULL);
  }
  if(expr->data.expr.expr2 != NULL) {
      symbol_add_from_expression(table, expr->data.expr.expr2, NULL);
  }
}

void symbol_add_from_comma_expr(struct symbol_table *table, struct node *comma_expr, struct type **type) {
  assert(NODE_COMMA_EXPR == comma_expr->kind);
  printf("------------- Comma expr.. %d\n", type == NULL);
  if(comma_expr->data.comma_expr.expr != NULL) {
      symbol_add_from_expression(table, comma_expr->data.comma_expr.expr, type);
  }
  if(comma_expr->data.comma_expr.assignment_expr != NULL) {
    symbol_add_from_expression(table, comma_expr->data.comma_expr.assignment_expr, type);
  }
}

void symbol_add_from_initialized_decl_list(struct symbol_table *table,
					   struct node *initialized_decl_list,
					   struct type **type) {
  if(initialized_decl_list->data.initialized_decl_list.initialized_decl_list != NULL) {
      symbol_add_from_expression(table, initialized_decl_list->data.initialized_decl_list.initialized_decl_list, type);
  }
  if(initialized_decl_list->data.initialized_decl_list.initialized_decl != NULL) {
    symbol_add_from_expression(table, initialized_decl_list->data.initialized_decl_list.initialized_decl, type);
  }
}

void symbol_add_from_subscript_expr(struct symbol_table *table, struct node *subscript_expr) {
  assert(NODE_SUBSCRIPT_EXPR == subscript_expr->kind);
  if(subscript_expr->data.subscript_expr.postfix_expr != NULL) {
      symbol_add_from_expression(table, subscript_expr->data.subscript_expr.postfix_expr, NULL);
  }
  if(subscript_expr->data.subscript_expr.expr != NULL) {
      symbol_add_from_expression(table, subscript_expr->data.subscript_expr.expr, NULL);
  }
}

void symbol_add_from_function_call(struct symbol_table *table, struct node *function_call) {
    /* Since this is a function call, the function should already have a declaration or a definition
       by this time */
    assert(NODE_FUNCTION_CALL == function_call->kind);
    if(function_call->data.function_call.postfix_expr != NULL) {
        symbol_add_from_expression(table, function_call->data.function_call.postfix_expr, NULL);
    }
    if(function_call->data.function_call.expression_list != NULL) {
        symbol_add_from_expression(table, function_call->data.function_call.expression_list, NULL);
    }
}

void symbol_add_from_expression_list(struct symbol_table *table, struct node *expression_list) {
    assert(NODE_EXPRESSION_LIST == expression_list->kind);
    if(expression_list->data.expression_list.expression_list != NULL) {
      symbol_add_from_expression(table, expression_list->data.expression_list.expression_list, NULL);
    }
    if(expression_list->data.expression_list.assignment_expr != NULL) {
      symbol_add_from_expression(table, expression_list->data.expression_list.assignment_expr, NULL);
    }
}

void symbol_add_from_for_expr(struct symbol_table *table, struct node *for_expr) {
  assert(NODE_FOR_EXPR == for_expr->kind);
  if(for_expr->data.for_expr.initial_clause != NULL) {
      symbol_add_from_expression(table, for_expr->data.for_expr.initial_clause, NULL);
  }
  if(for_expr->data.for_expr.expr1 != NULL) {
      symbol_add_from_expression(table, for_expr->data.for_expr.expr1, NULL);
  }
  if(for_expr->data.for_expr.expr2 != NULL) {
      symbol_add_from_expression(table, for_expr->data.for_expr.expr2, NULL);
  }
}
void symbol_add_from_if_statement(struct symbol_table *table, struct node *if_statement) {
  assert(NODE_IF_STATEMENT == if_statement->kind);
  if(if_statement->data.if_statement.expr != NULL) {
      symbol_add_from_expression(table, if_statement->data.if_statement.expr, NULL);
  }
  if(if_statement->data.if_statement.if_statement != NULL) {
      symbol_add_from_expression(table, if_statement->data.if_statement.if_statement, NULL);
  }
  if(if_statement->data.if_statement.else_statement != NULL) {
      symbol_add_from_expression(table, if_statement->data.if_statement.else_statement, NULL);
  }
}

struct type *get_type_from_pointer_declarator(struct type *type_of_pointee) {
    return type_pointer(type_of_pointee);
}

void symbol_add_from_pointer_declarator(struct symbol_table *table, struct node *pointer_declarator,
    struct type **type) {
  struct type *pointer_declarator_type = NULL;
  assert(NODE_POINTER_DECLARATOR == pointer_declarator->kind);
  assert(pointer_declarator->data.pointer_declarator.declarator != NULL);
  pointer_declarator_type = get_type_from_pointer_declarator(*type);
  symbol_add_from_expression(table, pointer_declarator->data.pointer_declarator.declarator, &pointer_declarator_type);
  *type = pointer_declarator_type;
}

struct type *get_type_from_type_specifier(struct node *type_specifier) {
    int type_width = TYPE_WIDTH_INT;
    int type_kind = TYPE_BASIC;
    bool is_unsigned = false;
    int conversion_rank = CONVERSION_RANK_CHAR;

    assert(NODE_TYPE_SPECIFIER == type_specifier->kind);
    switch(type_specifier->data.type_specifier.kind_of_type_specifier) {
      case SIGNED_SHORT_INT:
        is_unsigned = true;
        type_width = TYPE_WIDTH_SHORT;
        conversion_rank = CONVERSION_RANK_SHORT;
        break;
      case UNSIGNED_SHORT_INT:
        type_width = TYPE_WIDTH_SHORT;
        is_unsigned = false;
        conversion_rank = CONVERSION_RANK_SHORT;
        break;
      case SIGNED_INT:
        type_width = TYPE_WIDTH_INT;
        is_unsigned = false;
        conversion_rank = CONVERSION_RANK_INT;
        break;
      case UNSIGNED_INT:
        type_width = TYPE_WIDTH_INT;
        is_unsigned = true;
        conversion_rank = CONVERSION_RANK_INT;
        break;
      case SIGNED_LONG_INT:
        type_width = TYPE_WIDTH_LONG;
        is_unsigned = false;
        conversion_rank = CONVERSION_RANK_LONG;
        break;
      case UNSIGNED_LONG_INT:
        type_width = TYPE_WIDTH_LONG;
        is_unsigned = true;
        conversion_rank = CONVERSION_RANK_INT;
        break;
      case CHARACTER_TYPE:
      case SIGNED_CHARACTER_TYPE:
      case UNSIGNED_CHARACTER_TYPE:
        type_width = TYPE_WIDTH_CHAR;
        /* In our implementation, only signed characters are used */
        is_unsigned = false;
        conversion_rank = CONVERSION_RANK_CHAR;
        break;
      case VOID_TYPE:
        type_kind = TYPE_VOID;
        break;
      default:
        assert(0);
        break;

    }

    if(type_kind == TYPE_BASIC) {
        return type_basic(is_unsigned, type_width, conversion_rank);
    } else {
        return type_void();
    }
}

void symbol_add_from_cast_expr(struct symbol_table *table, struct node *cast_expr) {
  assert(NODE_CAST_EXPR == cast_expr->kind);
  if(cast_expr->data.cast_expr.cast_expr != NULL) {
      symbol_add_from_expression(table, cast_expr->data.cast_expr.cast_expr, NULL);
  }
}

void symbol_add_from_decl(struct symbol_table *table, struct node *decl) {
    struct type *type = NULL;
    assert(NODE_DECL == decl->kind);
    /* decl_specifier doesn't need to be associated with a type. Hence we
     * pass in NULL
     */
    symbol_add_from_expression(table, decl->data.decl.decl_specifier, NULL);

    type = get_type_from_type_specifier(decl->data.decl.decl_specifier);
    symbol_add_from_expression(table, decl->data.decl.init_decl_list, &type);
}

void symbol_add_from_parameter_decl(struct symbol_table *table, struct node *parameter_decl,
                                    struct type **type) {
    assert(NODE_PARAMETER_DECL == parameter_decl->kind);
    /* decl_specifier doesn't need to be associated with a type. Hence we
     * pass in NULL
     */
    symbol_add_from_expression(table, parameter_decl->data.parameter_decl.type_specifier, NULL);

    *type = get_type_from_type_specifier(parameter_decl->data.parameter_decl.type_specifier);
    if(parameter_decl->data.parameter_decl.declarator != NULL) {
        symbol_add_from_expression(table, parameter_decl->data.parameter_decl.declarator, type);
    }
}

void symbol_add_from_parameter_list(struct symbol_table *table,
                                    struct node *parameter_list,
                                    struct type **function_type) {
    struct type *parameter_type = NULL;
    assert(NODE_PARAMETER_LIST == parameter_list->kind);

    if(parameter_list->data.parameter_list.parameter_list != NULL) {
        symbol_add_from_parameter_list(table, parameter_list->data.parameter_list.parameter_list,
                                       function_type);
    }

    if((*function_type)->data.function.function_symbol_table != NULL) {
        /* This is a function definition. Add the parameters to the symbol table too */
        symbol_add_from_parameter_decl((*function_type)->data.function.function_symbol_table,
                                   parameter_list->data.parameter_list.parameter_decl, &parameter_type);
    } else {
        /* This is just a declaration. No need to add the parameters to the symbol table. xxx */
        symbol_add_from_expression((*function_type)->data.function.function_symbol_table,
                                   parameter_list->data.parameter_list.parameter_decl, &parameter_type);
    }
    symbol_add_to_function_parameter_list((*function_type), parameter_type);
}

void symbol_add_from_function_declarator_from_definition(struct symbol_table *table,
                                                         struct node *function_declarator,
                                                         struct type **function_type) {
    assert(NODE_FUNCTION_DECLARATOR == function_declarator->kind);
    assert((*function_type)->data.function.function_symbol_table != NULL);

    /* Append the parameter_list into the function-type symbol as well */
    symbol_add_from_parameter_list(table,
                                   function_declarator->data.function_declarator.parameter_list,
                                   function_type);
    symbol_add_from_expression(table, function_declarator->data.function_declarator.direct_declarator,
                               function_type);
}

void symbol_add_from_function_def_specifier(struct symbol_table *table, struct node *function_def_specifier, struct type **function_type) {
    assert(NODE_FUNCTION_DEF_SPECIFIER == function_def_specifier->kind);

    (*function_type)->data.function.return_type =
        get_type_from_type_specifier(function_def_specifier->data.function_def_specifier.decl_specifier);

    /* The following won't do anything since we don't need to add type_specifiers to our symbol table */
    symbol_add_from_expression(table, function_def_specifier->data.function_def_specifier.decl_specifier, NULL);
    if(function_def_specifier->data.function_def_specifier.declarator->kind != NODE_FUNCTION_DECLARATOR) {
        symbol_table_num_errors++;
        printf("ERROR: Syntax error! Incorrect function definition\n");
    } else {
        symbol_add_from_function_declarator_from_definition(table,
                                                            function_def_specifier->data.function_def_specifier.declarator,
                                                            function_type);
    }
}

void symbol_add_from_function_declarator(struct symbol_table *table, struct node *function_declarator,
                                         struct type **return_type) {
    struct type *function_type = type_function(*return_type);
    assert(NODE_FUNCTION_DECLARATOR == function_declarator->kind);
    function_type->data.function.function_symbol_table =  malloc(sizeof(struct symbol_table));

    /* Append the parameter_list into the function-type symbol as well */
    symbol_add_from_parameter_list(table,
                                   function_declarator->data.function_declarator.parameter_list,
                                   &function_type);
    symbol_add_from_expression(table, function_declarator->data.function_declarator.direct_declarator,
                               &function_type);
}

unsigned long symbol_get_array_size_from_constant_expr(struct node *constant_expr) {
    if(constant_expr->kind == NODE_BINARY_OPERATION) {
        if(constant_expr->data.binary_operation.left_operand->kind != NODE_NUMBER ||
           constant_expr->data.binary_operation.right_operand->kind != NODE_NUMBER) {
            /* xxx: Error! We can't evaluate this expression.. */
            symbol_table_num_errors++;
            printf("Input to array size cannot be evaluated at compile-time!\n");
            return 0;
        } else {
            /* Evaluate the expression */
            unsigned long left_operand = constant_expr->data.binary_operation.left_operand->data.number.value;
            unsigned long right_operand = constant_expr->data.binary_operation.right_operand->data.number.value;
            unsigned long result = 10000; /* Garbage value to realize that our value is unset */
            switch (constant_expr->data.binary_operation.operation) {
              case(BINOP_MULTIPLICATION):
                result = (left_operand * right_operand);
                break;
              case(BINOP_DIVISION):
                result = (left_operand / right_operand);
                break;
              case(BINOP_ADDITION):
                result = (left_operand + right_operand);
                break;
              case(BINOP_SUBTRACTION):
                result = (left_operand - right_operand);
                break;
              case(BINOP_REMAINDER):
                result = (left_operand % right_operand);
                break;
              case(BINOP_ASSIGN):
                /* Array input cannot be assignment*/
              case(BINOP_ASSIGN_PLUS_EQUAL):
              case(BINOP_ASSIGN_MINUS_EQUAL):
              case(BINOP_ASSIGN_ASTERISK_EQUAL):
              case(BINOP_ASSIGN_SLASH_EQUAL):
              case(BINOP_ASSIGN_PERCENT_EQUAL):
              case(BINOP_ASSIGN_LESS_LESS_EQUAL):
              case(BINOP_ASSIGN_GREATER_GREATER_EQUAL):
              case(BINOP_ASSIGN_AMPERSAND_EQUAL):
              case(BINOP_ASSIGN_CARET_EQUAL):
              case(BINOP_ASSIGN_VBAR_EQUAL):
                symbol_table_num_errors++;
                printf("Incorrect input to array size!\n");
                result = 0;
                break;
                /* Different kinds of logical/bitwise expressions
                 */
              case(BINOP_LOGICAL_OR_EXPR):
                result = left_operand || right_operand;
                break;
              case(BINOP_LOGICAL_AND_EXPR):
                result = left_operand && right_operand;
                break;
              case(BINOP_BITWISE_OR_EXPR):
                result = left_operand | right_operand;
                break;
              case(BINOP_BITWISE_XOR_EXPR):
                result = left_operand ^ right_operand;
                break;
              case(BINOP_BITWISE_AND_EXPR):
                result = left_operand & right_operand;
                break;

                /* Different kinds of equality operators
                 */
              case(BINOP_IS_EQUAL_TO):
                result = (left_operand == right_operand);
                break;
              case(BINOP_NOT_EQUAL_TO):
                result = (left_operand != right_operand);
                break;

                /* Different relational, shift, additive
                 * and multiplicative operators
                 */
              case(BINOP_LESS_THAN):
                result = (left_operand < right_operand);
                break;
              case(BINOP_LESS_THAN_OR_EQUAL_TO):
                result = (left_operand <= right_operand);
                break;
              case(BINOP_GREATER_THAN):
                result = (left_operand > right_operand);
                break;
              case(BINOP_GREATER_THAN_OR_EQUAL_TO):
                result = (left_operand >= right_operand);
                break;
              case(BINOP_SHIFT_LEFT):
                result = (left_operand << right_operand);
                break;
              case(BINOP_SHIFT_RIGHT):
                result = (left_operand >> right_operand);
                break;

                /* Sequential Evaluation */
              case(BINOP_SEQUENTIAL_EVALUATION):
                symbol_table_num_errors++;
                printf("Invalid input to array size!\n");
                result = 0;
            }
            return result;
        }
    } else if (constant_expr->kind == NODE_UNARY_OPERATION) {
        /* xxx*/
        return 0;
    } else if (constant_expr->kind == NODE_TERNARY_OPERATION) {
        /* xxx */
        return 0;
    } else if(constant_expr->kind == NODE_NUMBER) {
        return constant_expr->data.number.value;
    } else {
        symbol_table_num_errors++;
        printf("Unable to decipher constant expression..\n");
        return 0;
    }
}

void symbol_add_from_array_declarator(struct symbol_table *table, struct node *array_declarator,
                                         struct type **array_type) {
    unsigned long array_size = 0;
    assert(NODE_ARRAY_DECLARATOR == array_declarator->kind);
    if(NULL != array_declarator->data.array_declarator.constant_expr) {
        array_size = symbol_get_array_size_from_constant_expr(array_declarator->data.array_declarator.constant_expr);
    }
    (*array_type) = type_array(*array_type, array_size);

    symbol_add_from_expression(table, array_declarator->data.array_declarator.direct_declarator,
                               array_type);
}

void symbol_add_from_labeled_statement(struct symbol_table *table, struct node *labeled_statement) {
    struct type *label_type = type_label();
    assert(NODE_LABELED_STATEMENT == labeled_statement->kind);

    symbol_add_from_identifier_to_labels_list(table, labeled_statement->data.labeled_statement.identifier,
                                              label_type);
}

void symbol_add_from_compound_statement(struct symbol_table *table, struct node *compound_statement,
					bool part_of_function_definition) {
    assert(NODE_COMPOUND_STATEMENT == compound_statement->kind);

    if(part_of_function_definition) {
      symbol_add_from_expression(table,
				 compound_statement->data.compound_statement.declaration_or_statement_list,
				 NULL);
    } else {
      struct symbol_table *block_scope_symbol_table = malloc(sizeof(struct symbol_table));
      symbol_initialize_table(block_scope_symbol_table, BLOCK_SCOPE_SYMBOL_TABLE);
      block_scope_symbol_table->parent_symbol_table = table;
      symbol_add_from_expression(block_scope_symbol_table,
				 compound_statement->data.compound_statement.declaration_or_statement_list,
				 NULL);
    }
}

void symbol_add_from_function_definition(struct symbol_table *table, struct node *function_definition) {
    struct type *function_type = type_function(NULL);
    assert(NODE_FUNCTION_DEFINITION == function_definition->kind);
    function_type->data.function.function_symbol_table =  malloc(sizeof(struct symbol_table));
    symbol_initialize_table(function_type->data.function.function_symbol_table, FUNCTION_SCOPE_SYMBOL_TABLE);
    /* Link the symbol table of the function to the symbol table of the parent */
    function_type->data.function.function_symbol_table->parent_symbol_table = table;

    /* We need to make sure we pass in the right symbol table here.. */
    symbol_add_from_expression(table,
                               function_definition->data.function_definition.function_def_specifier,
                               &function_type);
    /* We need to make sure we pass in the right symbol table here.. */
    symbol_add_from_compound_statement(function_type->data.function.function_symbol_table,
				       function_definition->data.function_definition.compound_statement, true);

}

void symbol_add_from_expression(struct symbol_table *table, struct node *expression,
                                struct type **type) {
  switch (expression->kind) {
    case NODE_UNARY_OPERATION:
      symbol_add_from_unary_operation(table, expression);
      break;
    case NODE_BINARY_OPERATION:
      symbol_add_from_binary_operation(table, expression);
      break;
    case NODE_TERNARY_OPERATION:
      symbol_add_from_ternary_operation(table, expression);
      break;
    case NODE_STATEMENT:
      symbol_add_from_statement(table, expression);
      break;
  case NODE_STATEMENT_LIST:
      symbol_add_from_statement_list(table, expression);
      break;
    case NODE_IDENTIFIER:
      symbol_add_from_identifier(&table, expression, type);
      break;
    case NODE_NUMBER:
      break;
    case NODE_STRING:
      break;
    case NODE_EXPR:
      symbol_add_from_expr(table, expression);
      break;
    case NODE_TYPE_SPECIFIER:
      /* nothing to do here */
      break;
    case NODE_POINTER:
      break;
    case NODE_ABSTRACT_DECL:
      /* not sure that we need to add this to symbol table */
      break;
    case NODE_FOR_EXPR:
      symbol_add_from_for_expr(table, expression);
      break;
    case NODE_IF_STATEMENT:
      symbol_add_from_if_statement(table, expression);
      break;
    case NODE_POINTER_DECLARATOR:
      symbol_add_from_pointer_declarator(table, expression, type);
      break;
    case NODE_DECL:
      symbol_add_from_decl(table, expression);
      break;
    case NODE_PARAMETER_DECL:
      symbol_add_from_parameter_decl(table, expression, type);
      break;
    case NODE_FUNCTION_DEF_SPECIFIER:
      symbol_add_from_function_def_specifier(table, expression, type);
      break;
    case NODE_FUNCTION_DECLARATOR:
      symbol_add_from_function_declarator(table, expression, type);
      break;
    case NODE_PARAMETER_LIST:
      assert(0); /* Should never reach here */
      break;
    case NODE_ARRAY_DECLARATOR:
      symbol_add_from_array_declarator(table, expression, type);
      break;
    case NODE_LABELED_STATEMENT:
      /* A label can only exist at function scope */
      if(table->type_of_symbol_table == FILE_SCOPE_SYMBOL_TABLE) {
          symbol_table_num_errors++;
          printf("ERROR: Statement labels can only exist within function scopes\n");
      }
      symbol_add_from_labeled_statement(table, expression);
      break;
    case NODE_COMPOUND_STATEMENT:
      /* A label can only exist at function scope */
      if(table->type_of_symbol_table == FILE_SCOPE_SYMBOL_TABLE) {
          symbol_table_num_errors++;
          printf("ERROR: Compound Statements can only exist within function scopes\n");
      }
      symbol_add_from_compound_statement(table, expression, false);
      break;
    case NODE_FUNCTION_DEFINITION:
      symbol_add_from_function_definition(table, expression);
      break;
    case NODE_CAST_EXPR:
      symbol_add_from_cast_expr(table, expression);
      break;
    case NODE_FUNCTION_CALL:
      symbol_add_from_function_call(table, expression);
      break;
    case NODE_EXPRESSION_LIST:
      symbol_add_from_expression_list(table, expression);
      break;
    case NODE_SUBSCRIPT_EXPR:
      symbol_add_from_subscript_expr(table, expression);
      break;
    case NODE_COMMA_EXPR:
      symbol_add_from_comma_expr(table, expression, type);
      break;
   case NODE_INITIALIZED_DECL_LIST:
      symbol_add_from_initialized_decl_list(table, expression, type);
      break;
    default:
      assert(0);
      break;
  }
}

void symbol_add_from_translation_unit(struct symbol_table *table, struct node *translation_unit) {
  assert(NODE_TRANSLATION_UNIT == translation_unit->kind);
  if(NULL != translation_unit->data.translation_unit.translation_unit) {
    symbol_add_from_translation_unit(table, translation_unit->data.translation_unit.translation_unit);
  }
  if(NULL != translation_unit->data.translation_unit.top_level_decl) {
      symbol_add_from_expression(table, translation_unit->data.translation_unit.top_level_decl, NULL);
  }
}

/***********************
 * PRINT SYMBOL TABLES *
 ***********************/

void symbol_print_type(FILE *output, struct type *type) {
    int i = 0;
    struct symbol_list *symbol_list;
    switch(type->kind) {
      case TYPE_BASIC:
        fprintf(output, "      Type: ");
        if(type->data.basic.is_unsigned) {
            fprintf(output, "unsigned ");
        }
        switch(type->data.basic.width) {
          case(1):
            fprintf(output, "character\n");
            break;
          case(2):
            fprintf(output, "short\n");
            break;
          case(4):
            fprintf(output, "int\n");
            break;
        }
        break;
      case TYPE_VOID:
        fprintf(output, "      Type: Void Type \n");
        break;
      case TYPE_POINTER:
        fprintf(output, "      Type: Pointer Type \n");
        fprintf(output, "  *** Begin Pointee *** \n");
        symbol_print_type(output, type->data.pointer.pointee);
        fprintf(output, "  *** End Pointee *** \n");
        break;
      case TYPE_ARRAY:
        fprintf(output, "      Type: Array Type \n");
        fprintf(output, "*** Type of the array: *** \n");
        symbol_print_type(output, type->data.array.array_type);
        fprintf(output, "  *** End Type of Array *** \n");
        fprintf(output, "Array Size: %lu\n", type->data.array.array_size);
        break;
      case TYPE_FUNCTION:
        fprintf(output, "       Type: Function Type \n");
        fprintf(output, "*** Begin Return type: *** \n");
        symbol_print_type(output, type->data.function.return_type);
        fprintf(output, "*** End Return type: *** \n");
        fprintf(output, " Number of parameters: %d\n", type->data.function.number_of_parameters);
        symbol_list = type->data.function.function_symbol_table->variables;
        /* symbol_list = type->data.function.parameter_list; */
        for(i = 0; i < type->data.function.number_of_parameters; i++) {
            fprintf(output, " === Parameter %d: \n", i+1);
            symbol_print_type(output, symbol_list->symbol.result.type);
            symbol_list = symbol_list->next;
            fprintf(output, "===\n");
        }
        break;
      default:
        fprintf(output, "Type not defined yet! \n");
        assert(0);
        break;
    }
}

void symbol_print_table(FILE *output, struct symbol_table *table) {
  struct symbol_list *iter;

  fputs("symbol table:\n", output);

  for (iter = table->variables; NULL != iter; iter = iter->next) {
      fprintf(output, "  variable: %s $%p\n", iter->symbol.name, (void *)&iter->symbol);
    symbol_print_type(output, iter->symbol.result.type);
  }
  fputs("\n", output);
}
