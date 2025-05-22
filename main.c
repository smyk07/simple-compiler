#include <stdio.h>
#include <string.h>

#include "includes/data_structures.h"
#include "includes/utils.h"

#include "includes/codegen.h"
#include "includes/lexer.h"
#include "includes/parser.h"

int main(int argc, char *argv[]) {
  char *filename = argv[1];
  char *extracted_filename = scu_extract_name(filename);

  char *code_buffer = NULL;
  int code_buffer_len = scu_read_file(filename, &code_buffer);

  // Lexing
  dynamic_array tokens;
  dynamic_array_init(&tokens, sizeof(token));
  lexer_tokenize(code_buffer, code_buffer_len, &tokens);

  // Lexing test function
  // print_tokens(&tokens);

  // Parsing
  parser p;
  program_node program;
  parser_init(tokens, &p);
  parse_program(&p, &program);

  // Parsing test function
  // print_program(&program);

  // Semantic Analysis and Codegen
  freopen("output.asm", "w", stdout);
  program_asm(&program);
  fclose(stdout);

  // Assembler
  scu_assemble("output.asm", extracted_filename);

  return 0;

  printf("Hello\n");
}
