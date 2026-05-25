/*
 * lexer: for the SCULL language
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#ifndef LEXER_H
#define LEXER_H

#include "core/common.h"
#include "core/ds/dynamic_array.h"
#include "core/utils.h"

/*
 * @struct lexer: maintains state of the lexer for tokenizing the source buffer.
 */
typedef struct lexer {
  /*
   * Pointer to source buffer and its size in bytes.
   */
  const char *buffer;
  u64 buffer_len;

  /*
   * Data concerned with current lexer state.
   */
  u64 line;     // <-- tracks the current line
  u64 pos;      // <-- current position in buffer
  u64 read_pos; // <-- next read position (usually pos + 1)
  char ch;      // <-- character at buffer[read_pos]
} lexer;

/*
 * @brief: Tokenize a string buffer into a dynamic_array of tokens.
 *
 * @param buffer: string to be tokenized.
 * @param buffer_len size of buffer (in bytes).
 * @param tokens: dynamic_array of tokens (should be initialized).
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
scu_result lexer_tokenize(const char *buffer, u64 buffer_len,
                          dynamic_array *tokens, const char *include_dir);

#endif // !LEXER_H
