/*
 * file: utils.h
 * brief: Basic utility functions for the simple-compiler.
 */
#ifndef UTILS
#define UTILS

#define MAX_LEN 4096

char *scu_extract_name(const char *filename);

int scu_read_file(char *path, char **buffer);

char *scu_format_string(char *__restrict __format, ...);

void scu_psuccess(char *__restrict __format, ...);
void scu_pdebug(char *__restrict __format, ...);
void scu_pwarning(char *__restrict __format, ...);
void scu_perror(unsigned int *errors, char *__restrict __format, ...);

void scu_check_errors(unsigned int *errors);

void scu_assemble(char *asm_file, char *output_file, unsigned int *errors);

#endif
