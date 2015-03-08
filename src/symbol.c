#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "node.h"
#include "symbol.h"
#include "type.h"

int symbol_table_num_errors;

void symbol_initialize_table(struct symbol_table *table) {
  table->variables = NULL;
}

/**********************************************
 * WALK PARSE TREE AND ADD SYMBOLS INTO TABLE *
 **********************************************/
/*
 * This function is used to retrieve a symbol from a table.
 */
struct symbol *symbol_get(struct symbol_table *table, char name[]) {
  struct symbol_list *iter;
  for (iter = table->variables; NULL != iter; iter = iter->next) {
    if (!strcmp(name, iter->symbol.name)) {
      return &iter->symbol;
    }
  }
  return NULL;
}

struct symbol *symbol_put(struct symbol_table *table, char name[], 
                          struct type *type) {
  struct symbol_list *symbol_list;

  symbol_list = malloc(sizeof(struct symbol_list));
  assert(NULL != symbol_list);

  strncpy(symbol_list->symbol.name, name, MAX_IDENTIFIER_LENGTH);
  symbol_list->symbol.result.type = type;
  symbol_list->symbol.result.ir_operand = NULL;

  symbol_list->next = table->variables;
  table->variables = symbol_list;

  return &symbol_list->symbol;
}

void symbol_add_from_identifier(struct symbol_table *table, struct node *identifier,
                                struct type *type) {
  struct symbol *symbol;
  assert(NODE_IDENTIFIER == identifier->kind);

  symbol = symbol_get(table, identifier->data.identifier.name);
  if (NULL == symbol) {
      symbol = symbol_put(table, identifier->data.identifier.name, type);
  }
  identifier->data.identifier.symbol = symbol;
}

void symbol_add_from_expression(struct symbol_table *table, struct node *expression,
                                struct type *type);

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

  symbol_add_from_expression(table, statement->data.statement.statement, NULL);
  symbol_add_from_expression(table, statement->data.statement.expression, NULL);
}

void symbol_add_from_statement_list(struct symbol_table *table, struct node *statement_list) {
  assert(NODE_STATEMENT_LIST == statement_list->kind);

  if (NULL != statement_list->data.statement_list.init) {
    symbol_add_from_statement_list(table, statement_list->data.statement_list.init);
  }
  symbol_add_from_expression(table, statement_list->data.statement_list.statement, NULL);
}

void symbol_add_from_expr(struct symbol_table *table, struct node *expr) {
  assert(NODE_EXPR == expr->kind);
  symbol_add_from_expression(table, expr->data.expr.expr1, NULL);
  symbol_add_from_expression(table, expr->data.expr.expr2, NULL);
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
    struct type *type_of_pointee) {
  struct type *pointer_declarator_type = NULL;
  assert(NODE_POINTER_DECLARATOR == pointer_declarator->kind);
  assert(pointer_declarator->data.pointer_declarator.declarator != NULL); 
  pointer_declarator_type = get_type_from_pointer_declarator(type_of_pointee);
  symbol_add_from_expression(table, pointer_declarator->data.pointer_declarator.declarator, pointer_declarator_type); 
}

struct type *get_type_from_type_specifier(struct node *type_specifier) {
    int type_width = TYPE_WIDTH_INT;
    int type_kind = TYPE_BASIC;
    bool is_unsigned = false;
    assert(NODE_TYPE_SPECIFIER == type_specifier->kind);
    switch(type_specifier->data.type_specifier.kind_of_type_specifier) {
      case SIGNED_SHORT_INT:
        is_unsigned = true;
        type_width = TYPE_WIDTH_SHORT;
        break;
      case UNSIGNED_SHORT_INT:
        type_width = TYPE_WIDTH_SHORT;
        is_unsigned = false;
        break;
      case SIGNED_INT:
        type_width = TYPE_WIDTH_INT;
        is_unsigned = false;
        break;
      case UNSIGNED_INT:
        type_width = TYPE_WIDTH_INT;
        is_unsigned = true;
        break;
      case SIGNED_LONG_INT:
        type_width = TYPE_WIDTH_LONG;
        is_unsigned = false;
        break;
      case UNSIGNED_LONG_INT:
        type_width = TYPE_WIDTH_LONG;
        is_unsigned = true;
        break;
      case CHARACTER_TYPE:
      case SIGNED_CHARACTER_TYPE:
      case UNSIGNED_CHARACTER_TYPE:
        type_width = TYPE_WIDTH_CHAR;
        /* In our implementation, only signed characters are used */
        is_unsigned = false;
        break;
      case VOID_TYPE:
        type_kind = TYPE_VOID;
        break;
      default:
        assert(0);
        break;

    }

    if(type_kind == TYPE_BASIC) {
        return type_basic(is_unsigned, type_width);
    } else {
        return type_void();
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
    symbol_add_from_expression(table, decl->data.decl.init_decl_list, type);
}

void symbol_add_from_parameter_decl(struct symbol_table *table, struct node *parameter_decl) {
    struct type *type = NULL; 
    assert(NODE_PARAMETER_DECL == parameter_decl->kind);
    /* decl_specifier doesn't need to be associated with a type. Hence we 
     * pass in NULL 
     */
    symbol_add_from_expression(table, parameter_decl->data.parameter_decl.type_specifier, NULL);

    type = get_type_from_type_specifier(parameter_decl->data.parameter_decl.type_specifier);
    symbol_add_from_expression(table, parameter_decl->data.parameter_decl.declarator, type);
}

void symbol_add_from_function_def_specifier(struct symbol_table *table, struct node *function_def_specifier) {
    struct type *type = NULL; 
    assert(NODE_FUNCTION_DEF_SPECIFIER == function_def_specifier->kind);
    /* decl_specifier doesn't need to be associated with a type. Hence we 
     * pass in NULL 
     */
    symbol_add_from_expression(table, function_def_specifier->data.function_def_specifier.decl_specifier, NULL);

    type = get_type_from_type_specifier(function_def_specifier->data.function_def_specifier.decl_specifier);
    symbol_add_from_expression(table, function_def_specifier->data.function_def_specifier.declarator, 
                               type_function(type));
}

void symbol_add_from_expression(struct symbol_table *table, struct node *expression,
                                struct type *type) {
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
      symbol_add_from_identifier(table, expression, type);
      break;
    case NODE_NUMBER:
      break;
    case NODE_STRING:
      break;
    case NODE_EXPR:
      symbol_add_from_expr(table, expression);
      break;
    case NODE_TYPE_SPECIFIER:
      /* not sure that we need to add this to symbol table */
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
      symbol_add_from_parameter_decl(table, expression);
      break;
    case NODE_FUNCTION_DEF_SPECIFIER:
      symbol_add_from_function_def_specifier(table, expression);
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
    switch(type->kind) {
      case TYPE_BASIC:
        fprintf(output, "      type: Basic Type \n"); 
        break;
      case TYPE_VOID:
        fprintf(output, "      type: Void Type \n"); 
        break;
      case TYPE_POINTER:
        fprintf(output, "      type: Pointer Type \n"); 
        fprintf(output, "  *** Begin Pointee *** \n");
        symbol_print_type(output, type->data.pointer.pointee);
        fprintf(output, "  *** End Pointee *** \n");
        break;
      case TYPE_FUNCTION:
        fprintf(output, "       type: Function Type \n"); 
        fprintf(output, "*** Begin Return type: *** \n"); 
        symbol_print_type(output, type->data.function.return_type);
        fprintf(output, "*** End Return type: *** \n"); 
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
      fprintf(output, "  variable: %s$%p\n", iter->symbol.name, (void *)&iter->symbol);
    symbol_print_type(output, iter->symbol.result.type); 
  }
  fputs("\n", output);
}
