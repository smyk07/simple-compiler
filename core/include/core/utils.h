/*
 * utils: basic utility functions for the SCULL Language
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#ifndef UTILS_H
#define UTILS_H

#include "core/common.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * @enum scu_result: global result type
 */
typedef enum scu_result {
  SCU_SUCCESS = 0,

  // cli/io
  SCU_ERR_ARGS,
  SCU_ERR_IO,

  // frontend pipeline
  SCU_ERR_LEX,
  SCU_ERR_PARSE,
  SCU_ERR_SEMA,

  // backend pipeline
  SCU_ERR_BACKEND,
  SCU_ERR_CODEGEN,
  SCU_ERR_LINK,
} scu_result;

#define SCU_TRY(expr)                                                          \
  do {                                                                         \
    scu_result r = (expr);                                                     \
    if (r != SCU_SUCCESS)                                                      \
      return r;                                                                \
  } while (0)

/*
 * @brief: allocates memory with error checking.
 *
 * @param size: number of bytes to allocate.
 *
 * @return pointer to the beginning of the allocated memory.
 */
void *scu_checked_malloc(u64 size);

/*
 * @brief: re-allocates memory with error checking
 *
 * @param ptr: pointer to a previously allocated memory block.
 * @param size: number of bytes to allocate.
 *
 * @return pointer to the beginning of the allocated memory.
 */
void *scu_checked_realloc(void *ptr, u64 size);

/*
 * @brief: return filename without the extension.
 *
 * @param filename: filename to extract the string from.
 *
 * @return the extracted filename string, without the extension
 */
char *scu_extract_name(const char *filename);

/*
 * @brief: read the contents of a file and store them in a buffer.
 *
 * @param path: path to file
 * @param buffer: pointer to a string where the contents of the file are to be
 * stored.
 * @param out_size: size of the buffer in bytes to be stored
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
scu_result scu_read_file(const char *path, char **buffer, u64 *out_size);

/*
 * @brief: formats a string with variable arguments.
 *
 * @param __format: a format string containing format specifiers.
 * @param ...: variable arguments corresponsing to the format specifiers in
 * __format.
 *
 * @return pointer to formatted string on success.
 */
char *scu_format_string(char *__restrict __format, ...);

/*
 * @brief: print a formatted success message.
 *
 * @param __format: a format string containing format specifiers.
 * @param ...: variable arguments corresponsing to the format specifiers in
 * __format.
 */
void scu_psuccess(char *__restrict __format, ...);

/*
 * @brief: print a formatted debug message.
 *
 * @param __format: a format string containing format specifiers.
 * @param ...: variable arguments corresponsing to the format specifiers in
 * __format.
 */
void scu_pdebug(char *__restrict __format, ...);

/*
 * @brief: print a formatted warning message.
 *
 * @param __format: a format string containing format specifiers.
 * @param ...: variable arguments corresponsing to the format specifiers in
 * __format.
 */
void scu_pwarning(char *__restrict __format, ...);

/*
 * @brief: print a formatted error message.
 *
 * @param __format: a format string containing format specifiers.
 * @param ...: variable arguments corresponsing to the format specifiers in
 * __format.
 */
void scu_perror(char *__restrict __format, ...);

/*
 * @brief: marks a code path as unreachable
 *
 * @param msg: a string literal describing the unreachable condition
 */
#ifdef NDEBUG
#define scu_punreachable(msg) __builtin_unreachable()
#else
#define scu_punreachable(msg)                                                  \
  do {                                                                         \
    fprintf(stderr,                                                            \
            "\033[1;31m[INTERNAL ERROR] \033[0m %s\n"                          \
            "  at %s:%d (%s)\n"                                                \
            "  this is a compiler bug.\n",                                     \
            msg, __FILE__, __LINE__, __func__);                                \
    abort();                                                                   \
  } while (0)
#endif

#endif // UTILS_H
