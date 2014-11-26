#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "node.h"
#include "symbol.h"
#include "type.h"
#include "ir.h"
#include "mips.h"

FILE *yyin;
int yyparse();
extern int yynerrs;


FILE *error_output;

struct node *root_node;

int main(int argc, char **argv) {
  FILE *input, *output;
  int result;
  struct symbol_table symbol_table;

  /* Figure out whether we're using stdin/stdout or file in/file out. */
  if (argc < 2 || !strcmp("-", argv[1])) {
    input = stdin;
  } else {
    input = fopen(argv[1], "r");
  }

  if (argc < 3 || !strcmp("-", argv[2])) {
    output = stdout;
  } else {
    output = fopen(argv[2], "w");
  }

  error_output = stderr;

  /* Tell lex where to get input. */
  yyin = input;

  result = yyparse();
  if (yynerrs > 0) {
    result = 1;
  }
  switch (result) {
    case 0:
      /* Successful parse. */
      break;

    case 1:
      fprintf(stderr, "parser encountered %d error%s\n", yynerrs, (yynerrs == 1 ? "" : "s"));
      return result;

    case 2:
      fprintf(stderr, "parser ran out of memory\n");
      return result;
  }

  symbol_initialize_table(&symbol_table);
  symbol_add_from_statement_list(&symbol_table, root_node);
  if (symbol_table_num_errors > 0) {
    fprintf(stderr, "symbol table pass encountered %d error%s\n", symbol_table_num_errors, (symbol_table_num_errors == 1 ? "" : "s"));
    return 3;
  }
  fprintf(stderr, "================= SYMBOLS ================\n");
  symbol_print_table(stderr, &symbol_table);

  type_assign_in_statement_list(root_node);
  if (type_checking_num_errors > 0) {
    fprintf(stderr, "type checking pass encountered %d error%s\n", type_checking_num_errors, (type_checking_num_errors == 1 ? "" : "s"));
    return 4;
  }
  fprintf(stderr, "=============== PARSE TREE ===============\n");
  node_print_statement_list(stderr, root_node);

  ir_generate_for_statement_list(root_node);
  fprintf(stderr, "=================== IR ===================\n");
  /* ir_print_section(stderr, root_node); */

  if (ir_generation_num_errors > 0) {
    fprintf(stderr, "IR generation pass encountered %d error%s\n", ir_generation_num_errors, (ir_generation_num_errors == 1 ? "" : "s"));
    return 5;
  }
  fprintf(stderr, "================== MIPS ==================\n");
  mips_print_program(output, root_node->ir);
  fputs("\n\n", output);

  return 0;
}
