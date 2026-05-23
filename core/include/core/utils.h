/*
 * utils: basic utility functions for the SCULL Language
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#ifndef UTILS_H
#define UTILS_H

#include "core/common.h"

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
 *
 * @return number of bytes read
 */
u32 scu_read_file(const char *path, char **buffer);

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
 * @brief: exit the compiler pipeline if errors are found.
 */
void scu_check_errors();

#endif // UTILS_H
