%option yylineno
%option nounput

%{
/*
 * scanner.l
 *
 * This file contains the specification for the (f)lex generated scanner
 * for the CSCI-E295 source language.
 *
 */

  #include <stdlib.h>
  #include <errno.h>
  #include <string.h>

  #define YYSTYPE struct node *
  #define YY_NO_INPUT

  #include "type.h"
  #include "node.h"
  #include "parser.h"

%}

newline         \n
ws              [ \t\v\f]

digit           [[:digit:]]
letter          [[:alpha:]]

id              {letter}({letter}|{digit})*
number          {digit}+

%%

{newline}   /* do nothing; lex is counting lines */
{ws}        /* do nothing */

  /* operators begin */
\*          return ASTERISK;
\+          return PLUS;
-           return MINUS;
\/          return SLASH;
\=          return EQUAL;

\(          return LEFT_PAREN;
\)          return RIGHT_PAREN;
;           return SEMICOLON;
  /* operators end */

  /* constants begin */
{number}    yylval = node_number(yytext); return NUMBER;
  /* constants end */

  /* identifiers */
{id}        yylval = node_identifier(yytext, yyleng); return IDENTIFIER;

.           return -1;

%%
