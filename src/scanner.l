%option yylineno
%option nounput

%{
/*
 * scanner.l
 *
 * This file contains the specification for the (f)lex generated scanner
 * for the CSCI-E95 source language.
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

int str_count = 0;
char str_buf[MAX_STRING_LENGTH];
char *str_buf_ptr;
char *char_ptr;
int char_count = 0;
char char_val;
int error_val = 0;
%}
%x comment
%x string
%x character
newline         \n
ws              [ \t\v\f]

digit           [[:digit:]]
letter          [[:alpha:]]
underscore      _
id              ({letter}|{underscore})({letter}|{digit}|{underscore})*
number          {digit}+

graphical_char  [!-#%-/:-?[-_{-~]
other_grap_char $|@|`

%%

 /* comments begin */
"/*"	     BEGIN(comment);

<comment>{
	   [^*\n]*	/* eat anything that's not a '*' */
	   "*"+[^*/\n]*	/* eat up '*'s not followed by '/'s */
	   \n		/* ++line_num; */
	   "*"+"/"	BEGIN(INITIAL);
	   <<EOF>>      {
	                 fprintf(stderr,
				 "ERROR: unterminated comment\n");
			 BEGIN(INITIAL);
			 yyterminate();
	                }
}
 /* comments end */

 /* strings begin */
\"          /* Need to reset buffer too. */str_buf_ptr = str_buf; BEGIN(string);

<string>{
\"          { /* saw closing quote - all done */
                 BEGIN(INITIAL);
                 if(error_val > 0) {
                    /* Error was found */
                    error_val = 0; str_count = 0;
                     return -1;
                 }
                 yylval = node_string(str_buf, str_count);
		 str_count = 0;
                 return STRING;
            }

\n          {
		fprintf(stderr,
		"ERROR: Multiline strings not allowed!\n");
		error_val = 1;
                str_count = 0;
		return -1;
            }

 \\0         {
               *str_buf_ptr++ = 0;
               str_count++;
             }

\\[0-7]{1,3} {
              /* octal escape sequence */
              unsigned int result;

              (void) sscanf( yytext + 1, "%o", &result );

               if ( result > 0xff )
		 error_val = 1; return -1;/* error, constant is out-of-bounds */

               *str_buf_ptr++ = result;
	       str_count++;
              }

((\\0{digit}+)|(\\[1-9]{digit}*))[^\"]*     {
             /* not an octal sequence - bad escape sequence. error */
               fprintf(stderr,
		"ERROR: Unrecognized escape sequence %s\n", yytext);
                str_count = 0; error_val = 1;
		return -1;
             }

\\n  *str_buf_ptr++ = '\n'; str_count++;
\\t  *str_buf_ptr++ = '\t'; str_count++;
\\r  *str_buf_ptr++ = '\r'; str_count++;
\\b  *str_buf_ptr++ = '\b'; str_count++;
\\f  *str_buf_ptr++ = '\f'; str_count++;
\\v  *str_buf_ptr++ = '\v'; str_count++;
\\\\  *str_buf_ptr++ = '\\'; str_count++;
\\a  *str_buf_ptr++ = '\a'; str_count++;
\\\'  *str_buf_ptr++ = '\''; str_count++;
\\\"  *str_buf_ptr++ = '\"'; str_count++;
\\\?  *str_buf_ptr++ = '\?'; str_count++;

\\(.|\n)  {
              fprintf(stderr, "ERROR: Unrecognized escape code %s\n", yytext);
              error_val = 1; return -1;
          }

[^\\\n\"]+   {
                 char *yptr = yytext;
                 while ( *yptr ) {
                       *str_buf_ptr++ = *yptr++;
                       str_count++;
                       }
             }
<<EOF>>      {
                 fprintf(stderr,
			 "ERROR: unterminated string\n");
		 BEGIN(INITIAL); yyterminate();
             }
}
 /*strings end */

{newline}   /* do nothing; lex is counting lines */
{ws}        putchar(' '); /* insert a single space */

  /* operators begin */
\*          return ASTERISK;
\(          return LEFT_PAREN;
\)          return RIGHT_PAREN;
;           return SEMICOLON;
\[          return LEFT_SQUARE;
\]           return RIGHT_SQUARE;
\{           return LEFT_CURLY;
\}           return RIGHT_CURLY;
"&"           return AMPERSAND;
\^           return CARET;
:           return COLON;
,           return COMMA;
\=          return EQUAL;
!           return EXCLAMATION;
\>           return GREATER;
\<           return LESS;
-           return MINUS;
%           return PERCENT;
\+           return PLUS;
\/          return SLASH;
\?           return QUESTION;
~           return TILDE;
\|           return VBAR;
\&\&          return AMPERSAND_AMPERSAND;
&\=          return AMPERSAND_EQUAL;
\*\=          return ASTERISK_EQUAL;
\^-          return CARET_EQUAL;
\=\=        return EQUAL_EQUAL;
\!\=        return EXCLAMATION_EQUAL;
\>\=        return GREATER_EQUAL;
\>\>        return GREATER_GREATER;
\>\>\=      return GREATER_GREATER_EQUAL;
\<\=        return LESS_EQUAL;
\<\<        return LESS_LESS;
\<\<\=      return LESS_LESS_EQUAL;
-\=         return MINUS_EQUAL;
--          return MINUS_MINUS;
%\=         return PERCENT_EQUAL;
\+\=        return PLUS_EQUAL;
\+\+        return PLUS_PLUS;
\/\=        return SLASH_EQUAL;
\|\=        return VBAR_EQUAL;
\|\|        return VBAR_VBAR;
  /* operators end */

  /* reserved words */
do          return DO;
for         return FOR;
return      return RETURN;
break       return BREAK;
short       return SHORT;
else        return ELSE;
goto        return GOTO;
signed      return SIGNED;
unsigned    return UNSIGNED;
char        return CHAR;
if          return IF;
void        return VOID;
int         return INT;
continue    return CONTINUE;
long        return LONG;
while       return WHILE;
  /* reserved words end */

  /* constants begin */
0{number}+  {
                 fprintf(stderr,
			 "ERROR: octal numbers not allowed!\n");
		 return -1;
             }
0x{number}+  {
                 fprintf(stderr,
			 "ERROR: hexadecimal numbers not allowed!\n");
		 return -1;
             }

{number}    yylval = node_number(yytext); return NUMBER;
{number}{letter}+   {
                       fprintf(stderr,
			 "ERROR: integer suffixes not allowed!\n");
                          return -1;
                     }
  /* constants end */


  /* char begin */
\'         char_count = 0; BEGIN(character);
<character>{
\'       {
              if(char_count > 1) {
		/* error. */ char_count = 0; BEGIN(INITIAL); return -1;
              } else if (error_val > 0) {
                char_count = 0; error_val = 0; BEGIN(INITIAL); return -1;
              } else {
                char_count = 0; yylval = node_character(char_val);
		BEGIN(INITIAL); return NUMBER;
	      }
          }
[^\\\n\'] {
              char_count++; char_val = *yytext;
          }

(\\[0-3][0-7]{0,2})|(\\[0-9]{1,2}) {
              /* octal escape sequence */
              unsigned int result;

              (void) sscanf( yytext + 1, "%o", &result);
	      if (result > 0xff ) {
		return -1;/* error, constant is out-of-bounds */
	      }
              char_val = result; char_count++;
              }

(\\[0-9]{3,})|([4-9][0-9]{0,2}) {
                  fprintf(stderr, "ERROR: Unrecognized escape code %s\n", yytext); error_val = 1; return -1;
              }
\\n      char_val = '\n'; char_count++;
\\t  char_val = '\t'; char_count++;
\\r  char_val = '\r'; char_count++;
\\b  char_val = '\b'; char_count++;
\\f  char_val = '\f'; char_count++;
\\v  char_val = '\v'; char_count++;
\\\\  char_val = '\\'; char_count++;
\\a  char_val = '\a'; char_count++;
\\\'  char_val = '\''; char_count++;
\\\"  char_val = '\"'; char_count++;
\\\?  char_val = '\?'; char_count++;

\\(.|\n)  fprintf(stderr, "ERROR: Unrecognized escape code %s\n", yytext); error_val = 1; return -1;

<<EOF>>   printf("ERROR: unterminated character\n"); yyterminate();
}
  /* char end*/

  /* identifiers begin */
{id}        yylval = node_identifier(yytext, yyleng); return IDENTIFIER;
  /* identifiers end */

<<EOF>>    yyterminate();
.          printf("Unrecognized symbol %s\n", yytext); return -1;

%%
