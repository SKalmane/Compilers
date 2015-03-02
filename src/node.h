#ifndef _NODE_H
#define _NODE_H

#include <stdio.h>
#include <stdbool.h>

struct type;

#define MAX_IDENTIFIER_LENGTH               31
#define MAX_STRING_LENGTH                  509

/* ================================================== */
/* =========== Different kinds of nodes ============= */

#define NODE_NUMBER                          0
#define NODE_IDENTIFIER                      1
#define NODE_STRING                          2
#define NODE_UNARY_OPERATION                 3
#define NODE_BINARY_OPERATION                4
#define NODE_TERNARY_OPERATION               5
#define NODE_STATEMENT                       6
#define NODE_STATEMENT_LIST                  7
#define NODE_DECL                            8
#define NODE_TYPE_SPECIFIER                  9
#define NODE_POINTER                         10
#define NODE_EXPR                            11
#define NODE_ABSTRACT_DECL                   12
#define NODE_FOR_EXPR                        13
#define NODE_TRANSLATION_UNIT                14
#define NODE_IF_STATEMENT                    15
/* ================================================== */

/* ================================================== */
/* =========== Type specifier enums === ============= */

#define SIGNED_SHORT_INT                            0
#define SIGNED_INT                                  1
#define SIGNED_LONG_INT                             2
#define UNSIGNED_SHORT_INT                          3
#define UNSIGNED_INT                                4
#define UNSIGNED_LONG_INT                           5
#define CHARACTER_TYPE                              6
#define SIGNED_CHARACTER_TYPE                       7
#define UNSIGNED_CHARACTER_TYPE                     8
#define VOID_TYPE                                   9
/* ================================================== */

/* The two kinds of declarators are
 * pointer declarators and direct declarators
 */
#define POINTER_DECLARATOR                   0
#define DIRECT_DECLARATOR                    1

struct result {
  struct type *type;
  struct ir_operand *ir_operand;
};

struct node {
  int kind;
  int line_number;
  struct ir_section *ir;
  union {
    struct {
      unsigned long value;
      bool overflow;
      struct result result;
    } number;
    struct {
      char name[MAX_IDENTIFIER_LENGTH + 1];
      struct symbol *symbol;
    } identifier;
    struct {
      char name[MAX_STRING_LENGTH + 1];
      int length;
    } string;
    struct {
      int operation;
      struct node *the_operand;
      struct result result;
    } unary_operation;
    struct {
      int operation;
      struct node *left_operand;
      struct node *right_operand;
      struct result result;
    } binary_operation;
    struct {
      struct node *first_operand;
      struct node *second_operand;
      struct node *third_operand;
      struct result result;
    } ternary_operation;
    struct {
      struct node *statement;
      struct node *expression;
        int type_of_statement;
    } statement;
    struct {
      struct node *init;
      struct node *statement;
    } statement_list;
    struct {
      struct node *expr1;
      struct node *expr2;
      int type_of_expr;
    } expr;
    struct {
      struct node *decl_specifier;
      struct node *init_decl_list;
    } decl;
    struct {
      int kind_of_type_specifier;
    } type_specifier;
    struct {
      struct node *pointer;
    } pointer;
    struct {
      struct node *abstract_direct_declarator;
      struct node *expression;
      int type_of_abstract_decl;
    } abstract_decl;
    struct {
      struct node *initial_clause;
      struct node *expr1;
      struct node *expr2;
    } for_expr;
    struct {
      struct node *translation_unit;
      struct node *top_level_decl;
    } translation_unit;
    struct {
      struct node *expr;
      struct node *if_statement;
      struct node *else_statement;
    } if_statement;
  } data;
};


/* ======================================================*/
/* ================ Unary operations =================== */
/* Prefix operations */
#define UNARYOP_PREFIX_INCREMENT                          0
#define UNARYOP_PREFIX_DECREMENT                          1
#define UNARYOP_SIZEOF                                    2
#define UNARYOP_BITWISE_NOT                               3
#define UNARYOP_LOGICAL_NOT                               4
#define UNARYOP_NEGATION                                  5
#define UNARYOP_PLUS                                      6
#define UNARYOP_ADDRESS_OF                                7
#define UNARYOP_INDIRECTION                               8
#define UNARYOP_CASTING                                   9  /*(typename) */

/*Postfix operations */
#define UNARYOP_SUBSCRIPTING                             10  /* a[k] */
#define UNARYOP_FUNCTION_CALL                            11
#define UNARYOP_DIRECT_SELECTION                         13 /* a.data */
#define UNARYOP_INDIRECT_SELECTION                       14 /* a->data */
#define UNARYOP_POSTFIX_INCREMENT                        15
#define UNARYOP_POSTFIX_DECREMENT                        16
/* ======================================================*/
/* ============= End of Unary operations =============== */


/* ======================================================*/
/* =============== Binary operations =================== */
#define BINOP_MULTIPLICATION                              0
#define BINOP_DIVISION                                    1
#define BINOP_ADDITION                                    2
#define BINOP_SUBTRACTION                                 3
#define BINOP_REMAINDER                                   4
#define BINOP_ASSIGN                                      5
#define BINOP_ASSIGN_PLUS_EQUAL                           6
#define BINOP_ASSIGN_MINUS_EQUAL                          7
#define BINOP_ASSIGN_ASTERISK_EQUAL                       8
#define BINOP_ASSIGN_SLASH_EQUAL                          9
#define BINOP_ASSIGN_PERCENT_EQUAL                       10
#define BINOP_ASSIGN_LESS_LESS_EQUAL                     11
#define BINOP_ASSIGN_GREATER_GREATER_EQUAL               12
#define BINOP_ASSIGN_AMPERSAND_EQUAL                     13
#define BINOP_ASSIGN_CARET_EQUAL                         14
#define BINOP_ASSIGN_VBAR_EQUAL                          15
/* Different kinds of logical/bitwise expressions
 */
#define BINOP_LOGICAL_OR_EXPR                            16
#define BINOP_LOGICAL_AND_EXPR                           17
#define BINOP_BITWISE_OR_EXPR                            18
#define BINOP_BITWISE_XOR_EXPR                           19
#define BINOP_BITWISE_AND_EXPR                           20

/* Different kinds of equality operators
 */
#define BINOP_IS_EQUAL_TO                                21
#define BINOP_NOT_EQUAL_TO                               22

/* Different relational, shift, additive
 * and multiplicative operators
 */
#define BINOP_LESS_THAN                                  23
#define BINOP_LESS_THAN_OR_EQUAL_TO                      24
#define BINOP_GREATER_THAN                               25
#define BINOP_GREATER_THAN_OR_EQUAL_TO                   26
#define BINOP_SHIFT_LEFT                                 27
#define BINOP_SHIFT_RIGHT                                28                      

/* Sequential Evaluation */
#define BINOP_SEQUENTIAL_EVALUATION                      29 /* Comma */

/* ======================================================*/
/* ============ End of Binary operations =============== */

/* ======================================================*/
/* Types of expressions */
#define BASE_EXPR                                         0
#define SUBSCRIPT_EXPR                                    1
#define EXPRESSION_LIST                                   2
#define FUNCTION_CALL                                     3
#define CONCAT_EXPR                                       4
#define DECL_STATEMENT                                    5
#define COMMA_SEPARATED_STATEMENT                         6
/* ======================================================*/

/* ======================================================*/
/* Types of abstract declarations */
#define PARENTHESIZED_ABSTRACT_DECL                       0
#define SQUARE_BRACKETS_ABSTRACT_DECL                     1
/* ======================================================*/

/* ======================================================*/
/* Types of statements */
#define EXPRESSION_STATEMENT_TYPE                        0
#define LABELED_STATEMENT_TYPE                            1
#define COMPOUND_STATEMENT_TYPE                           2
#define WHILE_STATEMENT_TYPE                              3
#define DO_STATEMENT_TYPE                                 4
#define FOR_STATEMENT_TYPE                                5
#define ITERATIVE_STATEMENT_TYPE                          6
#define BREAK_STATEMENT_TYPE                              7
#define CONTINUE_STATEMENT_TYPE                           8
#define RETURN_STATEMENT_TYPE                             9
#define GOTO_STATEMENT_TYPE                              10
#define NULL_STATEMENT_TYPE                              11
/* ======================================================*/

/* Constructors */
struct node *node_number(char *text);
struct node *node_character(char text);
struct node *node_identifier(char *text, int length);
struct node *node_string(char *text, int length);
struct node *node_statement_list(struct node *list, struct node *item);
struct node *node_unary_operation(int operation,
                                   struct node *the_operand);
struct node *node_binary_operation(struct node *left_operand, int operation,
                                   struct node *right_operand);
struct node *node_ternary_operation(struct node *first_operand,
                                    struct node *second_operand,
                                    struct node *third_operand);
struct node *node_statement(struct node *statement, struct node *expression, 
                            int type_of_statement);
struct node *node_expr(struct node *expr1, struct node *expr2, int type_of_expr);

struct node *node_translation_unit(struct node *translation_unit, struct node *top_level_decl);
struct node *node_statement_list(struct node *init, struct node *statement);
struct node *node_decl(struct node *decl_specifier,
		       struct node *init_decl_list);
struct node *node_initialized_decl_list(struct node *initialized_decl_list,
					struct node* initialized_decl);
struct node *node_initialized_decl(struct node *declarator,
				   struct node *initializer);

struct node *node_declarator(int typeOfDeclarator);

struct node *node_pointer_declarator(struct node* pointer,
				  struct node* direct_declarator);

struct node *node_pointer(struct node *pointer);
struct node *node_parameter_list(struct node *parameter_list,
				   struct node *parameter_decl);
struct node *node_function_declarator(struct node *direct_declarator,
				      struct node *parameter_list);

struct node *node_type_specifier(int kind_of_type_specifier); 
struct node *node_for_expr(struct node *initial_clause, struct node *expr1, 
                           struct node *expr2); 

struct node *node_abstract_decl(struct node *abstract_direct_declarator, 
                                struct node *expression, int type_of_abstract_decl); 

struct node *node_if_statement(struct node *expr, struct node *if_statement, struct node *else_statement);

struct result *node_get_result(struct node *expression);

void node_print_statement_list(FILE *output, struct node *statement_list);
void node_print_translation_unit(FILE *output, struct node *translation_unit);
#endif
