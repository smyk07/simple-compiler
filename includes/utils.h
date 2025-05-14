#ifndef UTILS
#define UTILS

#define MAX_LEN 4096

int read_file(char *path, char **buffer);
char *extract_name(const char *filename);
void assemble(char *asm_file, char *output_file);

#endif
