#include <string.h>

#include "includes/data_structures.h"
#include "includes/utils.h"

#include "includes/codegen.h"
#include "includes/lexer.h"
#include "includes/parser.h"

int main(int argc, char *argv[]) {
  char *buffer = NULL;
  int buffer_len = read_file(argv[1], &buffer);

  // Lexing
  dynamic_array tokens;
  dynamic_array_init(&tokens, sizeof(token));
  lexer_tokenize(buffer, buffer_len, &tokens);

  // Parsing
  parser p;
  program_node program;
  parser_init(tokens, &p);
  parse_program(&p, &program);

  // Codegen
  program_asm(&program);

  return 0;
}
