#include <string.h>

#include "includes/data_structures.h"
#include "includes/utils.h"

#include "includes/lexer.h"

int main(int argc, char *argv[]) {
  char *buffer = NULL;
  int buffer_len = read_file(argv[1], &buffer);

  dynamic_array tokens;
  dynamic_array_init(&tokens, sizeof(token));

  lexer_tokenize(buffer, buffer_len, &tokens);

  for (unsigned int i = 0; i <= tokens.count - 1; i++) {
    token token;
    dynamic_array_get(&tokens, i, &token);
    print_token(token);
  }

  return 0;
}
