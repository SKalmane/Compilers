#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "node.h"
#include "symbol.h"
#include "type.h"
#include "ir.h"
#include "mips.h"


#define YYSTYPE struct node *
#include "scanner.h"
#include "parser.h"

int yyparse();
extern int yynerrs;

extern int errno;

struct node *root_node;

static void print_errors_from_pass(FILE *output, char *pass, int num_errors) {
  fprintf(output, "%s encountered %d %s.\n",
          pass, num_errors, (num_errors == 1 ? "error" : "errors"));
}

static void print_string(FILE *output, char *str, int length) {
  int i;
  for (i = 0; i < length; i++) {
    if (str[i] == 0) {
      fprintf(output, "\\0");
    } else {
      fprintf(output, "%c", str[i]);
    }
  }
}

int scan_only(FILE *output) {
  /* Begin scanning. */
  int num_errors = 0;
  int token = yylex();
  while (0 != token) {
    char *token_type, *token_name;

    /*
     * Print the line number. Use printf formatting and tabs to keep columns
     * lined up.
     */
    fprintf(output, "line = %-5d", yylineno);

    /*
     * Print the scanned text. Try to use formatting but give up instead of
     * truncating if the text is too long.
     */
    /*
    if (yyleng <= 20) {
      fprintf(output, "   text = %-20s", yytext);
    } else {
      fprintf(output, "   text = %s", yytext);
    }
    */

    if (token > 0) {
      /* Found a token! */

      /* Look up and print the token's name. */
      /* Print information about the token. */
      switch (token) {
        case CHAR:
          token_type = "rsvwd"; token_name = "CHAR"; break;
        case INT:
          token_type = "rsvwd"; token_name = "INT"; break;
        case LONG:
          token_type = "rsvwd"; token_name = "LONG"; break;
        case RETURN:
          token_type = "rsvwd"; token_name = "RETURN"; break;
        case SHORT:
          token_type = "rsvwd"; token_name = "SHORT"; break;
        case SIGNED:
          token_type = "rsvwd"; token_name = "SIGNED"; break;
        case UNSIGNED:
          token_type = "rsvwd"; token_name = "UNSIGNED"; break;
        case VOID:
          token_type = "rsvwd"; token_name = "VOID"; break;
        case CONTINUE:
          token_type = "rsvwd"; token_name = "CONTINUE"; break;
        case DO:
          token_type = "rsvwd"; token_name = "DO"; break;
        case WHILE:
          token_type = "rsvwd"; token_name = "WHILE"; break;
        case IF:
          token_type = "rsvwd"; token_name = "IF"; break;
        case ELSE:
          token_type = "rsvwd"; token_name = "ELSE"; break;
        case FOR:
          token_type = "rsvwd"; token_name = "FOR"; break;
        case GOTO:
          token_type = "rsvwd"; token_name = "GOTO"; break;
        case BREAK:
	  token_type = "rsvwd"; token_name = "BREAK"; break;
        case SEMICOLON:
          token_type = "op"; token_name = "SEMICOLON"; break;
        case ASTERISK:
          token_type = "op"; token_name = "ASTERISK"; break;
        case MINUS:
          token_type = "op"; token_name = "MINUS"; break;
        case PLUS:
          token_type = "op"; token_name = "PLUS"; break;
        case EQUAL:
          token_type = "op"; token_name = "EQUAL"; break;
        case SLASH:
          token_type = "op"; token_name = "SLASH"; break;
        case LEFT_PAREN:
          token_type = "op"; token_name = "LEFT_PAREN"; break;
        case RIGHT_PAREN:
          token_type = "op"; token_name = "RIGHT_PAREN"; break;
        case LEFT_SQUARE:
          token_type = "op"; token_name = "LEFT_SQUARE"; break;
        case RIGHT_SQUARE:
          token_type = "op"; token_name = "RIGHT_SQUARE"; break;
        case LEFT_CURLY:
          token_type = "op"; token_name = "LEFT_CURLY"; break;
        case RIGHT_CURLY:
          token_type = "op"; token_name = "RIGHT_CURLY"; break;
        case AMPERSAND:
          token_type = "op"; token_name = "AMPERSAND"; break;
        case CARET:
          token_type = "op"; token_name = "CARET"; break;
        case COLON:
          token_type = "op"; token_name = "COLON"; break;
        case COMMA:
          token_type = "op"; token_name = "COMMA"; break;
        case EXCLAMATION:
          token_type = "op"; token_name = "EXCLAMATION"; break;
        case GREATER:
          token_type = "op"; token_name = "GREATER"; break;
        case LESS:
          token_type = "op"; token_name = "LESS"; break;
        case PERCENT:
          token_type = "op"; token_name = "PERCENT"; break;
        case QUESTION:
          token_type = "op"; token_name = "QUESTION"; break;
        case TILDE:
          token_type = "op"; token_name = "TILDE"; break;
        case VBAR:
          token_type = "op"; token_name = "VBAR"; break;
        case AMPERSAND_AMPERSAND:
          token_type = "op"; token_name = "AMPERSAND_AMPERSAND"; break;
        case AMPERSAND_EQUAL:
          token_type = "op"; token_name = "AMPERSAND_EQUAL"; break;
        case ASTERISK_EQUAL:
          token_type = "op"; token_name = "ASTERISK_EQUAL"; break;
        case CARET_EQUAL:
          token_type = "op"; token_name = "CARET_EQUAL"; break;
        case EQUAL_EQUAL:
          token_type = "op"; token_name = "EQUAL_EQUAL"; break;
        case EXCLAMATION_EQUAL:
          token_type = "op"; token_name = "EXCLAMATION_EQUAL"; break;
        case GREATER_EQUAL:
          token_type = "op"; token_name = "GREATER_EQUAL"; break;
        case GREATER_GREATER:
          token_type = "op"; token_name = "GREATER_GREATER"; break;
        case GREATER_GREATER_EQUAL:
          token_type = "op"; token_name = "GREATER_GREATER_EQUAL"; break;
        case LESS_EQUAL:
          token_type = "op"; token_name = "LESS_EQUAL"; break;
        case LESS_LESS:
          token_type = "op"; token_name = "LESS_LESS"; break;
        case LESS_LESS_EQUAL:
          token_type = "op"; token_name = "LESS_LESS_EQUAL"; break;
        case MINUS_EQUAL:
          token_type = "op"; token_name = "MINUS_EQUAL"; break;
        case MINUS_MINUS:
          token_type = "op"; token_name = "MINUS_MINUS"; break;
        case PERCENT_EQUAL:
          token_type = "op"; token_name = "PERCENT_EQUAL"; break;
        case PLUS_EQUAL:
          token_type = "op"; token_name = "PLUS_EQUAL"; break;
        case PLUS_PLUS:
          token_type = "op"; token_name = "PLUS_PLUS"; break;
        case SLASH_EQUAL:
          token_type = "op"; token_name = "SLASH_EQUAL"; break;
        case VBAR_EQUAL:
          token_type = "op"; token_name = "VBAR_EQUAL"; break;
        case VBAR_VBAR:
          token_type = "op"; token_name = "VBAR_VBAR"; break;
        case NUMBER:
          token_type = "number"; token_name = "NUMBER"; break;

        case IDENTIFIER:
          token_type = "id"; token_name = "IDENTIFIER"; break;

        case STRING:
	  token_type = "str"; token_name = "STRING"; break;
        default:
          assert(0);
          break;
      }

    } else {
      token_type = "error"; token_name = "ERROR";
      num_errors++;
    }

    fprintf(output, "   %5s = %-20s", token_type, token_name);

    if (0 == strcmp("number", token_type)) {
      /* Print the type and value. */
      if(yylval->data.number.result.type == NULL) {
	/* assert(yylval->data.number.overflow);  */
	fprintf(output, "   type = %8s %-12s   value = %-10lu\n",
		"NUMBER", "OVERFLOW", yylval->data.number.value);
      } else {
	if(yylval->data.number.result.type->data.basic.is_unsigned) {
	  fprintf(output, "   type = %8s %-12s   value = %-10lu\n",
		  "UNSIGNED", "LONG", yylval->data.number.value);

	} else {
	  if(yylval->data.number.result.type->data.basic.width == TYPE_WIDTH_LONG) {
	    fprintf(output, "   type = %8s %-12s   value = %-10lu\n",
		    "SIGNED", "INT", yylval->data.number.value);
	  } else if(yylval->data.number.result.type->data.basic.width == TYPE_WIDTH_CHAR) {
	    fprintf(output, "   type = %10s        value = %-10c\n",
		    "CHARACTER", (char)yylval->data.number.value);
	  }
	}
      }
    } else if (0 == strcmp("id", token_type)) {
      fprintf(output, "\tToken type: Identifier\tName = %s\n", yylval->data.identifier.name);
    } else if (0 == strcmp("str", token_type)) {
      fprintf(output, "\tToken type: String\tName = ");
      print_string(output, yylval->data.string.name, yylval->data.string.length);
      fprintf(output, "\n");
    } else if (0 == strcmp("op", token_type)) {
      fprintf(output, "\tToken type: Operand\tName = %s\n", token_name);
    } else if (0 == strcmp("rsvwd", token_type)) {
      fprintf(output, "\tToken type: Keyword\tName = %s\n", token_name);
    } else {
      fputs("\n", output);
    }

    token = yylex();
  }
  return num_errors;
}

int main(int argc, char **argv) {
  FILE *output;
  int result;
  struct symbol_table symbol_table;
  char *stage;
  int opt;

  output = NULL;
  stage = "mips";
  while (-1 != (opt = getopt(argc, argv, "o:s:"))) {
    switch (opt) {
      case 'o':
        output = fopen(optarg, "w");
        if (NULL == output) {
          fprintf(stdout, "Could not open output file %s: %s", optarg, strerror(errno));
          return -1;
        }
        break;
      case 's':
        stage = optarg;
        break;
    }
  }
  /* Figure out whether we're using stdin/stdout or file in/file out. */
  if (optind >= argc) {
    yyin = stdin;
  } else if (optind == argc - 1) {
    yyin = fopen(argv[optind], "r");
  } else {
    fprintf(stdout, "Expected 1 input file, found %d.\n", argc - optind);
    return -1;
  }

  if (NULL == output) {
    output = fopen("output.s", "w");
  }

  if (0 == strcmp("scanner", stage)) {
    int num_errors = scan_only(stdout);
    if (num_errors > 0) {
      print_errors_from_pass(stdout, "Scanner", yynerrs);
      return 2;
    } else {
      return 0;
    }
  }

  result = yyparse();
  if (yynerrs > 0) {
    result = 1;
  }
  switch (result) {
    case 0:
      /* Successful parse. */
      break;

    case 1:
      print_errors_from_pass(stdout, "Parser", yynerrs);
      return 1;

    case 2:
      fprintf(stdout, "Parser ran out of memory.\n");
      return 2;
  }
  if (0 == strcmp("parser", stage)) {
    fprintf(stdout, "=============== PARSE TREE ===============\n");
    /* node_print_statement_list(stdout, root_node); */
    node_print_translation_unit(stdout, root_node);
    return 0;
  }

  symbol_initialize_table(&symbol_table, FILE_SCOPE_SYMBOL_TABLE);
  /* symbol_add_from_statement_list(&symbol_table, root_node); */
  symbol_add_from_translation_unit(&symbol_table, root_node);
  if (symbol_table_num_errors > 0) {
    print_errors_from_pass(stdout, "Symbol table", symbol_table_num_errors);
    return 3;
  }
  fprintf(stdout, "================= SYMBOLS ================\n");
  symbol_print_table(stdout, &symbol_table);
  if (0 == strcmp("symbol", stage)) {
    fprintf(stdout, "=============== PARSE TREE ===============\n");
    /* node_print_statement_list(stdout, root_node); */
    node_print_translation_unit(stdout, root_node);
    return 0;
  }

  /* xxx: type_assign_in_translation_unit(root_node); */
  if (type_checking_num_errors > 0) {
    print_errors_from_pass(stdout, "Type checking", type_checking_num_errors);
    return 4;
  }
  if (0 == strcmp("type_checking", stage)) {  
    return 0;
  }

  fprintf(stdout, "=============== PARSE TREE ===============\n");
  node_print_translation_unit(stdout, root_node);
  if (0 == strcmp("parser", stage)) {
    return 0;
  }

  ir_generate_for_program(root_node);
  if (ir_generation_num_errors > 0) {
    print_errors_from_pass(stdout, "IR generation", ir_generation_num_errors);
    return 5;
  }
  fprintf(stdout, "=================== IR ===================\n");
  ir_print_section(stdout, root_node->ir);
  if (0 == strcmp("ir", stage)) {
    return 0;
  }

  fprintf(stdout, "================== MIPS ==================\n");
  mips_print_program(stdout, root_node->ir);
  fputs("\n\n", stdout);

  mips_print_program(output, root_node->ir);
  fputs("\n\n", output);

  return 0;
}
