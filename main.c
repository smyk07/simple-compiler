#include <stdio.h>
#include <string.h>

#include "includes/data_structures.h"
#include "includes/semantic.h"
#include "includes/utils.h"

#include "includes/codegen.h"
#include "includes/lexer.h"
#include "includes/parser.h"

int main(int argc, char *argv[]) {
  char *filename = argv[1];
  char *extracted_filename = scu_extract_name(filename);

  char *code_buffer = NULL;
  int code_buffer_len = scu_read_file(filename, &code_buffer);

  int errors = 0;

  // Lexing
  dynamic_array tokens;
  dynamic_array_init(&tokens, sizeof(token));
  lexer_tokenize(code_buffer, code_buffer_len, &tokens, &errors);

  // Lexing test function
  // print_tokens(&tokens);

  // Parsing
  parser p;
  program_node program;
  parser_init(tokens, &p);
  parse_program(&p, &program, &errors);

  // Parsing test function
  // print_program(&program);

  // Semantic Analysis
  dynamic_array variables;
  dynamic_array_init(&variables, sizeof(variable));

  check_semantics(&program.instrs, &variables, &errors);

  // Codegen
  freopen("output.asm", "w", stdout);
  program_asm(&program, &variables);
  fclose(stdout);

  // Assembler
  scu_assemble("output.asm", extracted_filename);

  return 0;

  printf("Hello\n");
}
