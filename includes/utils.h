/*
 * utils: Basic utility functions for the simple-compiler.
 */

#ifndef UTILS
#define UTILS

#include <stddef.h>

/*
 * @brief: allocates memory with error checking.
 *
 * @param size: number of bytes to allocate.
 *
 * @return pointer to the beginning of the allocated memory.
 */
void *scu_checked_malloc(size_t size);

/*
 * @brief: re-allocates memory with error checking
 *
 * @param ptr: pointer to a previously allocated memory block.
 * @param size: number of bytes to allocate.
 *
 * @return pointer to the beginning of the allocated memory.
 */
void *scu_checked_realloc(void *ptr, size_t size);

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
 * @param error_count counter variable to increment when an error is
 * encountered.
 *
 * @return number of bytes read
 */
int scu_read_file(const char *path, char **buffer, unsigned int *error_count);

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
 * @param errors counter variable to increment when an error is encountered.
 * @param __format: a format string containing format specifiers.
 * @param ...: variable arguments corresponsing to the format specifiers in
 * __format.
 */
void scu_perror(unsigned int *errors, char *__restrict __format, ...);

/*
 * @brief: exit the compiler pipeline if errors are found.
 *
 * @param errors counter variable to increment when an error is encountered.
 */
void scu_check_errors(unsigned int *errors);

#endif
