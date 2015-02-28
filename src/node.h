#ifndef _NODE_H
#define _NODE_H

#include <stdio.h>
#include <stdbool.h>

struct type;

#define MAX_IDENTIFIER_LENGTH               31
#define MAX_STRING_LENGTH                  509
#define NODE_NUMBER                          0
#define NODE_IDENTIFIER                      1
#define NODE_STRING                          2
#define NODE_BINARY_OPERATION                3
#define NODE_EXPRESSION_STATEMENT            4
#define NODE_STATEMENT_LIST                  5
#define NODE_DECL                            6
#define NODE_TYPE_SPECIFIER                  7
#define NODE_SIGNED_TYPE_SPECIFIER           8
#define NODE_UNSIGNED_TYPE_SPECIFIER         9
#define NODE_CHARACTER_TYPE_SPECIFIER        10
#define NODE_INITIALIZED_DECL_LIST           11
#define NODE_INITIALIZED_DECL                12
#define NODE_DECLARATOR                      13
#define NODE_POINTER_DECLARATOR              14
#define NODE_POINTER                         15
#define NODE_DIRECT_DECLARATOR               16
#define NODE_FUNCTION_DECLARATOR             17
#define NODE_PARAMETER_LIST                  18

#define NODE_EXPR                            19
#define ASSIGNMENT_EXPR                      20
#define CONDITIONAL_EXPR                     21
#define LOGICAL_OR_EXPR                      22
#define LOGICAL_AND_EXPR                     23
#define BITWISE_OR_EXPR                      24
#define BITWISE_XOR_EXPR                     25
#define BITWISE_AND_EXPR                     26

#define SIGNED                               0
#define UNSIGNED                             1
#define CHARACTER                            2

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
      struct node *left_operand;
      struct node *right_operand;
      struct result result;
    } binary_operation;
    struct {
      struct node *expression;
    } expression_statement;
    struct {
      struct node *init;
      struct node *statement;
    } statement_list;
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
    } signed_type_specifier;
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

/* Binary operations */
#define BINOP_MULTIPLICATION           0
#define BINOP_DIVISION                 1
#define BINOP_ADDITION                 2
#define BINOP_SUBTRACTION              3
#define BINOP_ASSIGN                   4

/* Constructors */
struct node *node_number(char *text);
struct node *node_character(char text);
struct node *node_identifier(char *text, int length);
struct node *node_string(char *text, int length);
struct node *node_statement_list(struct node *list, struct node *item);
struct node *node_binary_operation(int operation, struct node *left_operand,
                                   struct node *right_operand);
struct node *node_expression_statement(struct node *expression);
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

#endif
