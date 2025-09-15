/*
 * fasm: contains functions for the fasm assembler.
 */

#ifndef FASM
#define FASM

/*
 * @brief: helper function to assemble the generated fasm assembly '.s' file to
 * an executable binary.
 *
 * @param asm_file: name of the generated assembly file.
 * @param output_file: name to be given to the output executable binary.
 */
void fasm_assemble(const char *asm_file, const char *output_file);

#endif // !FASM
