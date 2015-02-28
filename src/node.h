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
#define NODE_EXPRESSION_STATEMENT            6
#define NODE_STATEMENT_LIST                  7
#define NODE_DECL                            8
#define NODE_TYPE_SPECIFIER                  9
#define NODE_SIGNED_TYPE_SPECIFIER           10
#define NODE_UNSIGNED_TYPE_SPECIFIER         11
#define NODE_CHARACTER_TYPE_SPECIFIER        12
#define NODE_INITIALIZED_DECL_LIST           13
#define NODE_INITIALIZED_DECL                14
#define NODE_DECLARATOR                      15
#define NODE_POINTER_DECLARATOR              16
#define NODE_POINTER                         17
#define NODE_DIRECT_DECLARATOR               18
#define NODE_FUNCTION_DECLARATOR             19
#define NODE_PARAMETER_LIST                  20
#define NODE_EXPR                  21
/* ================================================== */

#define SIGNED_TYPE                            0
#define UNSIGNED_TYPE                          1
#define CHARACTER_TYPE                         2

/* Define the various types used to 
 * construct the type_specifier nodes
 */
#define NONE_INT                             0
#define SHORT_INT                            1
#define LONG_INT                             2

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
      struct node *expression;
    } expression_statement;
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
      struct node *type_specifier_node;
      int typeOfSpecifier;
    } type_specifier;
    struct {
      int typeOfSignedSpecifier;
    } signed_type_specifier;
    struct {
      int typeOfUnsignedSpecifier;
    } unsigned_type_specifier;
    struct {
      int typeOfCharacterSpecifier;
    } character_type_specifier;
    struct {
      struct node *initialized_decl_list;
      struct node *initialized_decl;
    } initialized_decl_list;
    struct {
      struct node *declarator;
      struct node *initializer;
    } initialized_decl;
    struct {
      int typeOfDeclarator;
    } declarator;
    struct {
      struct node *pointer;
      struct node *direct_declarator;
    } pointer_declarator;
    struct {
      struct node *pointer;
    } pointer;
    struct {
      struct node *direct_declarator;
      struct node *parameter_list;
    } function_declarator; 
    struct {
      struct node *parameter_list;
      struct node *parameter_decl;
    } parameter_list; 
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
#define EXPRESSION_STATEMENT                              2
#define FUNCTION_CALL                                     3
#define CAST_EXPR                                         4
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
struct node *node_expression_statement(struct node *expression);
struct node *node_expr(struct node *expr1, struct node *expr2, int type_of_expr);

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

struct node *node_type_specifier(int typeOfTypeSpecifier,
				 struct node *type_specifier_node);
struct node *node_signed_type_specifier(int typeOfSignedSpecifier);
struct node *node_unsigned_type_specifier(int typeOfUnsignedSpecifier);
struct node *node_unsigned_type_specifier(int typeOfUnsignedSpecifier);
struct node *node_character_type_specifier(int typeOfCharacterSpecifier);

struct result *node_get_result(struct node *expression);

void node_print_statement_list(FILE *output, struct node *statement_list);

void node_print_assignment_expr(FILE *output, struct node *assignment_expr);

void node_print_expr(FILE *output, struct node *expr);
#endif
