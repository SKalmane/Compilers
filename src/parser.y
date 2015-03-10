%verbose
%debug

%{

  #include <stdio.h>
  #include "node.h"

  int yylex();
  extern int yylineno;
  void yyerror(char const *s);

  #define YYSTYPE struct node *
  #define YYERROR_VERBOSE

  extern struct node *root_node;

%}

%token IDENTIFIER NUMBER STRING

%token BREAK CHAR CONTINUE DO ELSE FOR GOTO IF
%token INT LONG RETURN SHORT SIGNED UNSIGNED VOID WHILE

%token LEFT_PAREN RIGHT_PAREN LEFT_SQUARE RIGHT_SQUARE LEFT_CURLY RIGHT_CURLY

%token AMPERSAND ASTERISK CARET COLON COMMA EQUAL EXCLAMATION GREATER
%token LESS MINUS PERCENT PLUS SEMICOLON SLASH QUESTION TILDE VBAR

%token AMPERSAND_AMPERSAND AMPERSAND_EQUAL ASTERISK_EQUAL CARET_EQUAL
%token EQUAL_EQUAL EXCLAMATION_EQUAL GREATER_EQUAL GREATER_GREATER
%token GREATER_GREATER_EQUAL LESS_EQUAL LESS_LESS LESS_LESS_EQUAL
%token MINUS_EQUAL MINUS_MINUS PERCENT_EQUAL PLUS_EQUAL PLUS_PLUS
%token SLASH_EQUAL VBAR_EQUAL VBAR_VBAR

%start program

%%

type_specifiers
  : SHORT
      { $$ = node_type_specifier(SIGNED_SHORT_INT);}
  | SHORT INT
      { $$ = node_type_specifier(SIGNED_SHORT_INT);}
  | SIGNED SHORT
      { $$ = node_type_specifier(SIGNED_SHORT_INT);}
  | SIGNED SHORT INT
      { $$ = node_type_specifier(SIGNED_SHORT_INT);}
  | INT
      { $$ = node_type_specifier(SIGNED_INT);}
  | SIGNED INT
      { $$ = node_type_specifier(SIGNED_INT);}
  | SIGNED
      { $$ = node_type_specifier(SIGNED_INT);}
  | LONG
      { $$ = node_type_specifier(SIGNED_LONG_INT);}
  | LONG INT
      { $$ = node_type_specifier(SIGNED_LONG_INT);}
  | SIGNED LONG
      { $$ = node_type_specifier(SIGNED_LONG_INT);}
  | SIGNED LONG INT
      { $$ = node_type_specifier(SIGNED_LONG_INT);}
  | UNSIGNED SHORT
      { $$ = node_type_specifier(UNSIGNED_SHORT_INT);}
  | UNSIGNED SHORT INT
      { $$ = node_type_specifier(UNSIGNED_SHORT_INT);}
  | UNSIGNED
      { $$ = node_type_specifier(UNSIGNED_INT);}
  | UNSIGNED INT
      { $$ = node_type_specifier(UNSIGNED_INT);}
  | UNSIGNED LONG
      { $$ = node_type_specifier(UNSIGNED_LONG_INT);}
  | UNSIGNED LONG INT
      { $$ = node_type_specifier(UNSIGNED_LONG_INT);}
  | CHAR
      { $$ = node_type_specifier(CHARACTER_TYPE);}
  | SIGNED CHAR
      { $$ = node_type_specifier(SIGNED_CHARACTER_TYPE);}
  | UNSIGNED CHAR
      { $$ = node_type_specifier(UNSIGNED_CHARACTER_TYPE);}
  | VOID
      { $$ = node_type_specifier(VOID_TYPE);}
;

multiplicative_expr
  : cast_expr
  | multiplicative_expr ASTERISK cast_expr
      { $$ = node_binary_operation($1, BINOP_MULTIPLICATION, $3); }
  | multiplicative_expr SLASH cast_expr
      { $$ = node_binary_operation($1, BINOP_DIVISION, $3); }
  | multiplicative_expr PERCENT cast_expr
      { $$ = node_binary_operation($1, BINOP_REMAINDER, $3); }
;

additive_expr
  : multiplicative_expr
  | additive_expr PLUS multiplicative_expr
      { $$ = node_binary_operation($1, BINOP_ADDITION, $3); }
  | additive_expr MINUS multiplicative_expr
      { $$ = node_binary_operation($1, BINOP_SUBTRACTION, $3); }
;

shift_expr
  : additive_expr
  | shift_expr LESS_LESS additive_expr
      { $$ = node_binary_operation($1, BINOP_SHIFT_LEFT, $3); }
  | shift_expr GREATER_GREATER additive_expr
      { $$ = node_binary_operation($1, BINOP_SHIFT_RIGHT, $3); }
;

relational_expr
  : shift_expr
  | relational_expr LESS shift_expr
      { $$ = node_binary_operation($1, BINOP_LESS_THAN, $3); }
  | relational_expr LESS_EQUAL shift_expr
      { $$ = node_binary_operation($1, BINOP_LESS_THAN_OR_EQUAL_TO, $3); }
  | relational_expr GREATER shift_expr
      { $$ = node_binary_operation($1, BINOP_GREATER_THAN, $3); }
  | relational_expr GREATER_EQUAL shift_expr
      { $$ = node_binary_operation($1, BINOP_GREATER_THAN_OR_EQUAL_TO, $3); }
;

equality_expr
  : relational_expr
  | equality_expr EQUAL_EQUAL relational_expr
      { $$ = node_binary_operation($1, BINOP_IS_EQUAL_TO, $3); }
  | equality_expr EXCLAMATION_EQUAL relational_expr
      { $$ = node_binary_operation($1, BINOP_NOT_EQUAL_TO, $3); }
;

bitwise_and_expr
  : equality_expr
  | bitwise_and_expr AMPERSAND equality_expr
      { $$ = node_binary_operation($1, BINOP_BITWISE_AND_EXPR, $3); }
;

bitwise_or_expr
  : bitwise_xor_expr
  | bitwise_or_expr  VBAR  bitwise_xor_expr
      { $$ = node_binary_operation($1, BINOP_BITWISE_OR_EXPR, $3); }
;

bitwise_xor_expr
  : bitwise_and_expr
  | bitwise_xor_expr CARET bitwise_and_expr
      { $$ = node_binary_operation($1, BINOP_BITWISE_XOR_EXPR, $3); }
;

logical_and_expr
  : bitwise_or_expr
  | logical_and_expr AMPERSAND_AMPERSAND bitwise_or_expr
      { $$ = node_binary_operation($1, BINOP_LOGICAL_AND_EXPR, $3); }

logical_or_expr
 : logical_and_expr
 | logical_or_expr VBAR_VBAR logical_and_expr
      { $$ = node_binary_operation($1, BINOP_LOGICAL_OR_EXPR, $3); }
;

parenthesized_expr
  : LEFT_PAREN expr RIGHT_PAREN
      { $$ = $2; }
;

constant
  : NUMBER
  | STRING
;

primary_expr
  : IDENTIFIER
  | parenthesized_expr
  | constant
;

subscript_expr
  : postfix_expr LEFT_SQUARE expr RIGHT_SQUARE
      { $$ = node_expr($1, $3, SUBSCRIPT_EXPR); }
;

expression_list
  : assignment_expr
  | expression_list COMMA assignment_expr
      { $$ = node_expr($1, $3, EXPRESSION_LIST); }
;

function_call
  : postfix_expr LEFT_PAREN expression_list RIGHT_PAREN
      { $$ = node_expr($1, $3, FUNCTION_CALL); }
  | postfix_expr LEFT_PAREN RIGHT_PAREN
      { $$ = node_expr($1, NULL, FUNCTION_CALL); }
;

postincrement_expr
  : postfix_expr PLUS_PLUS
  { $$ = node_unary_operation(UNARYOP_POSTFIX_INCREMENT, $1); }
;

postdecrement_expr
  : postfix_expr MINUS_MINUS
  { $$ = node_unary_operation(UNARYOP_POSTFIX_DECREMENT, $1); }
;

postfix_expr
  : primary_expr
  | subscript_expr
  | function_call
  | postincrement_expr
  | postdecrement_expr
;

type_name
  : type_specifiers
  | type_specifiers abstract_declarator
      { $$ = node_expr($1, $2, CONCAT_EXPR); }
;

cast_expr
  : unary_expr
  | unary_casting_expr cast_expr
      { $$ = node_expr($1, $2, CONCAT_EXPR); }
;

unary_casting_expr
  : LEFT_PAREN type_name RIGHT_PAREN
      { $$ = node_unary_operation(UNARYOP_CASTING, $2); }
;

unary_minus_expr
  : MINUS cast_expr
  { $$ = node_unary_operation(UNARYOP_NEGATION, $2); }
;

unary_plus_expr
  : PLUS cast_expr
      { $$ = node_unary_operation(UNARYOP_PLUS, $2); }
;

logical_negation_expr
  : EXCLAMATION cast_expr
      { $$ = node_unary_operation(UNARYOP_LOGICAL_NOT, $2); }
;

bitwise_negation_expr
  : TILDE cast_expr
      { $$ = node_unary_operation(UNARYOP_BITWISE_NOT, $2); }
;

address_expr
  : AMPERSAND cast_expr
      { $$ = node_unary_operation(UNARYOP_ADDRESS_OF, $2); }
;

indirection_expr
  : ASTERISK cast_expr
      { $$ = node_unary_operation(UNARYOP_INDIRECTION, $2); }
;

preincrement_expr
  : PLUS_PLUS unary_expr
      { $$ = node_unary_operation(UNARYOP_PREFIX_INCREMENT, $2); }
;

predecrement_expr
  : MINUS_MINUS unary_expr
      { $$ = node_unary_operation(UNARYOP_PREFIX_DECREMENT, $2); }
;

unary_expr
  : postfix_expr
  | unary_minus_expr
  | unary_plus_expr
  | logical_negation_expr
  | bitwise_negation_expr
  | address_expr
  | indirection_expr
  | preincrement_expr
  | predecrement_expr
;

assignment_expr
  : conditional_expr
  | unary_expr EQUAL assignment_expr
      { $$ = node_binary_operation($1, BINOP_ASSIGN, $3); }
  | unary_expr PLUS_EQUAL assignment_expr
      { $$ = node_binary_operation($1, BINOP_ASSIGN_PLUS_EQUAL, $3); }
  | unary_expr MINUS_EQUAL assignment_expr
      { $$ = node_binary_operation($1, BINOP_ASSIGN_MINUS_EQUAL, $3); }
  | unary_expr ASTERISK_EQUAL assignment_expr
      { $$ = node_binary_operation($1, BINOP_ASSIGN_ASTERISK_EQUAL, $3); }
  | unary_expr SLASH_EQUAL assignment_expr
      { $$ = node_binary_operation($1, BINOP_ASSIGN_SLASH_EQUAL, $3); }
  | unary_expr PERCENT_EQUAL assignment_expr
      { $$ = node_binary_operation($1, BINOP_ASSIGN_PERCENT_EQUAL, $3); }
  | unary_expr LESS_LESS_EQUAL assignment_expr
      { $$ = node_binary_operation($1, BINOP_ASSIGN_LESS_LESS_EQUAL, $3); }
  | unary_expr GREATER_GREATER_EQUAL assignment_expr
      { $$ = node_binary_operation($1, BINOP_ASSIGN_GREATER_GREATER_EQUAL, $3); }
  | unary_expr AMPERSAND_EQUAL assignment_expr
      { $$ = node_binary_operation($1, BINOP_ASSIGN_AMPERSAND_EQUAL, $3); }
  | unary_expr CARET_EQUAL assignment_expr
      { $$ = node_binary_operation($1, BINOP_ASSIGN_CARET_EQUAL, $3); }
  | unary_expr VBAR_EQUAL assignment_expr
      { $$ = node_binary_operation($1, BINOP_ASSIGN_VBAR_EQUAL, $3); }
;

comma_expr
  : assignment_expr
      { $$ = node_expr(NULL, $1, ASSIGNMENT_EXPR); }
  | expr COMMA assignment_expr
      { $$ = node_expr($1, $3, COMMA_SEPARATED_STATEMENT); }
;

expr
  : comma_expr
;

conditional_expr
  : logical_or_expr
  | logical_or_expr QUESTION expr COLON conditional_expr
      { $$ = node_ternary_operation($1, $3, $5); }
;

constant_expr
  : conditional_expr
;

abstract_direct_declarator
  : LEFT_PAREN abstract_declarator RIGHT_PAREN
      { $$ = node_abstract_decl(NULL, $2, PARENTHESIZED_ABSTRACT_DECL); }
  | LEFT_SQUARE RIGHT_SQUARE
      { $$ = node_abstract_decl(NULL, NULL, SQUARE_BRACKETS_ABSTRACT_DECL); }
  | LEFT_SQUARE constant_expr RIGHT_SQUARE
      { $$ = node_abstract_decl(NULL, $2, SQUARE_BRACKETS_ABSTRACT_DECL); }
  | abstract_direct_declarator LEFT_SQUARE RIGHT_SQUARE
      { $$ = node_abstract_decl($1, NULL, SQUARE_BRACKETS_ABSTRACT_DECL); }
  | abstract_direct_declarator LEFT_SQUARE constant_expr RIGHT_SQUARE
      { $$ = node_abstract_decl($1, $3, SQUARE_BRACKETS_ABSTRACT_DECL); }
;

abstract_declarator
  : pointer
  | abstract_direct_declarator
  | pointer abstract_direct_declarator
      {$$ = node_expr($1, $2, CONCAT_EXPR); }
;

parameter_decl
  : type_specifiers declarator
      { $$ = node_parameter_decl($1, $2); }
  | type_specifiers
  | type_specifiers abstract_declarator
      {$$ = node_expr($1, $2, CONCAT_EXPR); }
;

pointer
  : ASTERISK
      {$$ = node_pointer(NULL); }
  | ASTERISK pointer
      {$$ = node_pointer($2); }
;

parameter_list
  : parameter_decl
      { $$ = node_parameter_list(NULL, $1); }
  | parameter_list COMMA parameter_decl
      { $$ = node_parameter_list($1, $3); }
;

function_declarator
  : direct_declarator LEFT_PAREN parameter_list RIGHT_PAREN
      { $$ = node_function_declarator($1, $3); }
;

/* declarator */
/*   : pointer_declarator */
/*   | direct_declarator */
/* ; */

declarator
  : direct_declarator
  | ASTERISK declarator
      { $$ = node_pointer_declarator($2); }
;

simple_declarator
  : IDENTIFIER
;

array_declarator
  : direct_declarator LEFT_SQUARE RIGHT_SQUARE
      { $$ = node_abstract_decl($1, NULL, SQUARE_BRACKETS_ABSTRACT_DECL); } /* xxx Need to rename */
  | direct_declarator LEFT_SQUARE constant_expr RIGHT_SQUARE
      { $$ = node_abstract_decl($1, $3, SQUARE_BRACKETS_ABSTRACT_DECL); }
;

direct_declarator
  : simple_declarator
  | LEFT_PAREN declarator RIGHT_PAREN
      { $$ = $2; }
  | function_declarator
  | array_declarator
;

/* pointer_declarator */
/*   : pointer direct_declarator */
/*       {$$ = node_expr($1, $2, CONCAT_EXPR); } */
/* ; */

initialized_decl
  : declarator
;

initialized_decl_list
  : initialized_decl
  | initialized_decl_list COMMA initialized_decl
      { $$ = node_expr($1, $3, COMMA_SEPARATED_STATEMENT); }
;

decl_specifiers
  : type_specifiers
;

expression_statement
  : expr SEMICOLON
      { $$ = node_statement(NULL, $1, EXPRESSION_STATEMENT_TYPE); }
;

labeled_statement
  : IDENTIFIER COLON statement
      { $$ = node_statement($3, $1, LABELED_STATEMENT_TYPE); }
;

conditional_statement
  : IF LEFT_PAREN expr RIGHT_PAREN statement
      { $$ = node_if_statement($3, $5, NULL); }
  | IF LEFT_PAREN expr RIGHT_PAREN statement ELSE statement
      { $$ = node_if_statement($3, $5, $7); }
;

while_statement
  : WHILE LEFT_PAREN expr RIGHT_PAREN statement
      { $$ = node_statement($5, $3, WHILE_STATEMENT_TYPE); }
;

do_statement
  : DO statement WHILE LEFT_PAREN expr RIGHT_PAREN SEMICOLON
      { $$ = node_statement($2, $5, DO_STATEMENT_TYPE); }
;

initial_clause
  : expr
  | decl
;

for_expr
  : LEFT_PAREN SEMICOLON SEMICOLON RIGHT_PAREN
      { $$ = node_for_expr(NULL, NULL, NULL); }
  | LEFT_PAREN SEMICOLON SEMICOLON expr RIGHT_PAREN
      { $$ = node_for_expr(NULL, NULL, $4); }
  | LEFT_PAREN SEMICOLON expr SEMICOLON RIGHT_PAREN
      { $$ = node_for_expr(NULL, $3, NULL); }
  | LEFT_PAREN SEMICOLON expr SEMICOLON expr RIGHT_PAREN
      { $$ = node_for_expr(NULL, $3, $5); }
  | LEFT_PAREN initial_clause SEMICOLON SEMICOLON RIGHT_PAREN
      { $$ = node_for_expr($2, NULL, NULL); }
  | LEFT_PAREN initial_clause SEMICOLON SEMICOLON expr RIGHT_PAREN
      { $$ = node_for_expr($2, NULL, $5); }
  | LEFT_PAREN initial_clause SEMICOLON expr SEMICOLON RIGHT_PAREN
      { $$ = node_for_expr($2, $4, NULL); }
  | LEFT_PAREN initial_clause SEMICOLON expr SEMICOLON expr RIGHT_PAREN
      { $$ = node_for_expr($2, $4, $6); }
;

for_statement
  : FOR for_expr statement
      { $$ = node_statement($3, $2, FOR_STATEMENT_TYPE); }
;

iterative_statement
  : while_statement
  | do_statement
  | for_statement
;

break_statement
  : BREAK SEMICOLON
      { $$ = node_statement(NULL, NULL, BREAK_STATEMENT_TYPE); }
;

continue_statement
  : CONTINUE SEMICOLON
      { $$ = node_statement(NULL, NULL, CONTINUE_STATEMENT_TYPE); }
;

return_statement
  : RETURN SEMICOLON
      { $$ = node_statement(NULL, NULL, RETURN_STATEMENT_TYPE); }
  | RETURN expr SEMICOLON
      { $$ = node_statement(NULL, $2, RETURN_STATEMENT_TYPE); }
;

goto_statement
  : GOTO IDENTIFIER SEMICOLON
      { $$ = node_statement(NULL, $2, GOTO_STATEMENT_TYPE); }
;

null_statement
  : SEMICOLON
      { $$ = node_statement(NULL, NULL, NULL_STATEMENT_TYPE); }
;

statement
  : expression_statement
  | labeled_statement
  | compound_statement
  | conditional_statement
  | iterative_statement
  | break_statement
  | continue_statement
  | return_statement
  | goto_statement
  | null_statement
;

declaration_or_statement
  : decl
  | statement
;

declaration_or_statement_list
  : declaration_or_statement
  | declaration_or_statement_list declaration_or_statement
      { $$ = node_statement_list($1, $2); }
;

decl
  : decl_specifiers initialized_decl_list SEMICOLON
      { $$ = node_decl($1, $2); }
;

function_def_specifier
  : decl_specifiers declarator
      { $$ = node_function_def_specifier($1, $2); }
;

compound_statement
  : LEFT_CURLY RIGHT_CURLY
      { $$ = node_statement(NULL, NULL, COMPOUND_STATEMENT_TYPE); }
  | LEFT_CURLY declaration_or_statement_list RIGHT_CURLY
      { $$ = node_statement($2, NULL, COMPOUND_STATEMENT_TYPE); }
;

function_definition
  : function_def_specifier compound_statement
      { $$ = node_expr($1, $2, CONCAT_EXPR); }
;

top_level_decl
  : decl
  | function_definition
;

translation_unit
  : top_level_decl
      { $$ = node_translation_unit(NULL, $1); }
  | translation_unit top_level_decl
      { $$ = node_translation_unit($1, $2); }
;

program
  : translation_unit
  { root_node = node_translation_unit($1, NULL); }
;

%%

void yyerror(char const *s) {
  fprintf(stderr, "ERROR at line %d: %s\n", yylineno, s);
}
