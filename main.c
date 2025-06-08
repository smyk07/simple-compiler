#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "includes/data_structures.h"
#include "includes/utils.h"

#include "includes/codegen.h"
#include "includes/lexer.h"
#include "includes/parser.h"
#include "includes/semantic.h"

char *parse_arguments(int argc, char *argv[], int *debug) {
  if (argc > 1) {
    if (strcmp(argv[1], "--cdebug") == 0 || strcmp(argv[1], "-cd") == 0) {
      *debug = 1;
      return argv[2];
    } else {
      return argv[1];
    }
  } else {
    printf("Simple Compiler - Just as the name suggests\n");
    printf("Usage: ./compiler [OPTIONS] <filename>\n\n");
    printf("Options:\n");
    printf("--cdebug OR -cd \t Run in compiler debug mode - prints lexer\n");
    printf("                \t and parser debug statements.\n");
    exit(1);
  }
}

int main(int argc, char *argv[]) {
  int errors = 0;
  int debug = 0;

  char *filename = parse_arguments(argc, argv, &debug);
  char *extracted_filename = scu_extract_name(filename);

  char *code_buffer = NULL;
  int code_buffer_len = scu_read_file(filename, &code_buffer);

  // Lexing
  dynamic_array tokens;
  dynamic_array_init(&tokens, sizeof(token));
  lexer_tokenize(code_buffer, code_buffer_len, &tokens, &errors);

  // Lexing test function
  if (debug)
    print_tokens(&tokens);

  // Parsing
  parser p;
  program_node program;
  parser_init(tokens, &p);
  parse_program(&p, &program, &errors);

  // Parsing test function
  if (debug)
    print_program(&program);

  dynamic_array variables;
  dynamic_array_init(&variables, sizeof(variable));

  // Semantic Analysis
  check_semantics(&program.instrs, &variables, &errors);

  // Semantic Debug Statements
  if (debug)
    scu_pdebug("Semantic Analysis Complete\n");

  // Codegen & Assembler
  program_asm(&program, &variables, extracted_filename, &errors);

  // Restore STDOUT
  stdout = fopen("/dev/tty", "w");
  scu_psuccess("%s\n", filename);

  // Free memory
  fclose(stdout);

  free(extracted_filename);
  free(code_buffer);

  free_tokens(&tokens);
  free_if_instrs(&program);

  dynamic_array_free(&tokens);
  dynamic_array_free(&variables);
  dynamic_array_free(&program.instrs);

  return 0;
}
