/*
 * file: utils.h
 * brief: Basic utility functions for the simple-compiler.
 */
#ifndef UTILS
#define UTILS

#define MAX_LEN 4096

char *scu_extract_name(const char *filename);

int scu_read_file(char *path, char **buffer);

void scu_check_errors(int *errors);

void scu_perror(int *errors, char *__restrict __format, ...);

void scu_pwarning(char *__restrict __format, ...);

void scu_assemble(char *asm_file, char *output_file);

#endif
