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

 /* %start translation_unit */
%start program

%%

character_type_specifiers
  : CHAR
      {node_character_type_specifier(CHARACTER);}
  | SIGNED CHAR
      {node_character_type_specifier(SIGNED);}  
  | UNSIGNED CHAR
      {node_character_type_specifier(UNSIGNED);}    
;

unsigned_type_specifiers
  : UNSIGNED SHORT
      {node_unsigned_type_specifier(SHORT_INT);}
  | UNSIGNED SHORT INT
      {node_unsigned_type_specifier(SHORT_INT);}  
  | UNSIGNED
      {node_unsigned_type_specifier(NONE_INT);}
  | UNSIGNED INT
      {node_unsigned_type_specifier(NONE_INT);}
  | UNSIGNED LONG
      {node_unsigned_type_specifier(LONG_INT);}
  | UNSIGNED_LONG_INT
      {node_unsigned_type_specifier(LONG_INT);}  
;

signed_type_specifiers
  : SHORT
      {node_signed_type_specifier(SHORT_INT);}
  | SHORT INT
      {node_signed_type_specifier(SHORT_INT);}
  | SIGNED SHORT
      {node_signed_type_specifier(SHORT_INT);}
  | SIGNED SHORT INT
      {node_signed_type_specifier(SHORT_INT);}
  | INT
      {node_signed_type_specifier(NONE_INT);}
  | SIGNED INT
      {node_signed_type_specifier(NONE_INT);}
  | SIGNED
      {node_signed_type_specifier(NONE_INT);}
  | LONG
      {node_signed_type_specifier(LONG_INT);}
  | LONG INT
      {node_signed_type_specifier(LONG_INT);}  
  | SIGNED LONG
      {node_signed_type_specifier(LONG_INT);}  
  | SIGNED LONG INT
      {node_signed_type_specifier(LONG_INT);}  
;

type_specifiers
  : signed_type_specifiers
      { $$ = node_type_specifier(SIGNED, $1); }
  | unsigned_type_specifiers
      { $$ = node_type_specifier(UNSIGNED, $1); }  
  | character_type_specifiers
      { $$ = node_type_specifier(CHARACTER, $1); }  
;
void_type_specifier
  : VOID
;
decl_specifiers
  : type_specifiers
  | void_type_specifier
;

pointer
  : ASTERISK
      {$$ = node_pointer(NULL); }
  | ASTERISK pointer
      {$$ = node_pointer($2); }  
;

simple_declarator
  : IDENTIFIER
;

equality_op
  : EQUAL_EQUAL
  | EXCLAMATION_EQUAL
;

multiplicative_expr
: ( cast_expr )
  | ( multiplicative_expr mult_op cast_expr )
;

mult_op = ( ASTERISK )
          ( SLASH )
          ( PERCENT )
;

additive_expr = ( multiplicative_expr ) or
                ( additive_expr and add_op and multiplicative_expr )
;
add_op
  : ( PLUS )
  | ( MINUS )
;
shift_expr
  : ( additive_expr ) or 
  | ( shift_expr shift_op additive_expr )
;

shift_op
  : ( LESS_LESS )
  | ( GREATER_GREATER )
;

relational_expr
  : ( shift_expr )
  | ( relational_expr relational_op shift_expr )
;

relational_op
  : ( LESS )
  | ( LESS_EQUAL )
  | ( GREATER )
  | ( GREATER_EQUAL )
;
equality_expr
  : ( relational_expr )
  | ( equality_expr equality_op relational_expr )
;
bitwise_and_expr
  : ( equality_expr )
  | ( bitwise_and_expr AMPERSAND equality_expr )
;

bitwise_or_expr
  : ( bitwise_xor_expr )
  | ( bitwise_or_expr  |  bitwise_xor_expr )
;

bitwise_xor_expr
  : ( bitwise_and_expr ) or
  | ( bitwise_xor_expr ^ bitwise_and_expr)
;
logical_and_expr
  : bitwise_or_expr
  | logical_and_expr && bitwise_or_expr

logical_or_expr
 : logical_and_expr
 | logical_or_expr OR logical_and_expr
;

parenthesized_expr
  : LEFT_PAREN expr RIGHT_PAREN
      { $$ = $2; }
;

primary_expr
  : ( IDENTIFIER )
  | ( parenthesized_expr )
  /*   | ( CONSTANT ) xxx: Implement this.. */
;

subscript_expr
  : ( postfix_expr LEFT_SQUARE expr RIGHT_SQUARE )
;

expression_list
  : ( assignment_expr )
  | ( expression_list COMMA assignment_expr )
;

function_call
  : ( postfix_expr LEFT_PAREN expression_list RIGHT_PAREN )
  | ( postfix_expr LEFT_PAREN RIGHT_PAREN )
;

postincrement_expr
  : ( postfix_expr PLUS_PLUS )
;

postdecrement_expr
  : ( postfix_expr MINUS_MINUS )
;

postfix_expr
  : ( primary_expr )
  | ( subscript_expr )
  | ( function_call )
  | ( postincrement_expr )
  | ( postdecrement_expr )
;

cast_expr
  : ( unary_expr )
  | ( LEFT_PAREN type_name RIGHT_PAREN cast_expr )
;

unary_minus_expr
  : ( MINUS cast_expr )
;

unary_plus_expr
  : ( PLUS cast_expr )
;

logical_negation_expr
  : ( EXCLAMATION cast_expr )
;

bitwise_negation_expr
  : ( TILDE cast_expr )
;

address_expr
  : ( AMPERSAND cast_expr )
;

indirection_expr
  : ( ASTERISK cast_expr )
;

preincrement_expr
  : ( PLUS_PLUS unary_expr )
;

predecrement_expr
  : ( MINUS_MINUS unary_expr )
;

unary_expr
  : ( postfix_expr )
  | ( unary_minus_expr )
  | ( unary_plus_expr )
  | ( logical_negation_expr )
  | ( bitwise_negation_expr )
  | ( address_expr )
  | ( indirection_expr )
  | ( preincrement_expr )
  | ( predecrement_expr )
;

assignment_expr
  : ( conditional_expr )
  | ( unary_expr assignment_op assignment_expr )
;

assignment_op
  : EQUALS
  | PLUS_EQUAL
  | MINUS_EQUAL
  | ASTERISK_EQUAL
  | SLASH_EQUAL
  | PERCENT_EQUAL
  | LESS_LESS_EQUAL
  | GREATER_GREATER_EQUAL
  | AMPERSAND_EQUAL
  | CARET_EQUAL
  | VBAR_EQUAL
;
  
expr
  : ( assignment_expr )
  | ( expr COMMA assignment_expr ) 
;

conditional_expr
  : logical_or_expr /* xxx */
  | logical_or_expr QUESTION expr COLON conditional_expr
;

constant_expr
  : conditional_expr
;

abstract_direct_declarator
  : LEFT_PAREN abstract_declarator RIGHT_PAREN
      { $$ = $2; }
  | LEFT_SQUARE RIGHT_SQUARE /* xxx: Needs to be corrected */
  | LEFT_SQUARE constant_expr RIGHT_SQUARE
      { $$ = $2; }
  | abstract_direct_declarator LEFT_SQUARE RIGHT_SQUARE
  | abstract_direct_declarator LEFT_SQUARE constant_expr RIGHT_SQUARE
      { $$ = $3; } /* xxx: Needs to be corrected */
;

abstract_declarator
  : pointer
  | abstract_direct declarator
  | pointer abstract_direct_declarator
;

parameter_decl
  : type_specifiers declarator
  {$$ = node_parameter_decl($1, }
  | type_specifiers
  | type_specifiers abstract_declarator
;

parameter_list
  : parameter_decl
      {$$ = node_parameter_list(NULL, $2); }
  | parameter_list COMMA parameter_decl
      {$$ = node_parameter_list($1, $2); }
;

function_declarator
  : direct_declarator LEFT_PAREN parameter_list RIGHT_PAREN
      {$$ = node_function_declarator($1, $3); }
;

direct_declarator
  : simple_declarator
  | LEFT_PAREN declarator RIGHT_PAREN
      { $$ = $2; }
  | function_declarator
  | array_declarator
;

pointer_declarator
  : pointer direct_declarator
      {$$ = node_pointer_declarator($1, $2); }
;

declarator
  : pointer_declarator
      { $$ = node_declarator(POINTER_DECLARATOR); }
  | direct_declarator
      { $$ = node_declarator(DIRECT_DECLARATOR); }  
;

initialized_decl
  : declarator
      { $$ = node_initialized_decl($1, NULL); }
  | declarator EQUALS initializer
      { $$ = node_initialized_decl($1, initializer); }  
;

initialized_decl_list
  : initialized_decl
      { $$ = node_initialized_decl_list(NULL, $1); }
  | initialized_decl_list COMMA initialized_decl
      { $$ = node_initialized_decl_list($1, $3); }
;

decl
  : decl_specifiers initialized_decl_list SEMICOLON
      { $$ = node_decl($1, $2); }
;

top_level_decl
  : decl
    /*  | function_definition */
;

translation_unit
  : top_level_decl
      { root_node = $1; }
;

program
  : expr
          { root_node = $1; }
;

%%

void yyerror(char const *s) {
  fprintf(stderr, "ERROR at line %d: %s\n", yylineno, s);
}

