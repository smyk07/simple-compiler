/*
 * cstate: Per-binary compilation state, groups all variables and options for
 * one build unit.
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#ifndef CSTATE_H
#define CSTATE_H

#include "core/common.h"
#include "core/ds/arena.h"
#include "core/ds/dynamic_array.h"
#include "core/utils.h"

#include "sclc/backend/backend.h"

/*
 * @enum opt_level: represents optimization levels
 */
typedef enum opt_level {
  OPT_O0 = 0, // no optimization
  OPT_O1,     // basic
  OPT_O2,     // default
  OPT_O3,     // aggressive
  OPT_Os,     // optimize for size
  OPT_Oz      // optimize for minimum size
} opt_level;

/*
 * @struct coptions: represents the options described in the command when the
 * binary is executed.
 */
typedef struct coptions {
  opt_level opt_level;

  bool verbose;

  // output control
  bool explicit_output_specified;
  bool compile_only;
  bool emit_llvm;
  bool emit_asm;

  // input control
  bool include_dir_specified;
  bool target_specified;
} coptions;

/*
 * @struct cstate: represents the compiler state.
 */
typedef struct cstate {
  coptions options;

  char *llvm_target_triple;

  /* input */
  char *include_dir;
  mem_arena file_arena;
  dynamic_array files;

  /* output */
  char *output_filepath;
  dynamic_array obj_file_list;

  backend backend;
} cstate;

/*
 * @brief: Initialize a already allocated compiler state from CLI arguments.
 *
 * @param cst: pointer to an initialized cst object
 * @param argc: count of args
 * @param argv: array of arguments (string)
 *
 * @return: SCU_SUCCESS on success, or the first error encountered
 */
scu_result cstate_init(cstate *cst, u32 argc, char *argv[]);

/*
 * @brief: runs the full compilation pipeline for each file in a cstate
 *
 * @param cst: pointer to an initialized cst object
 *
 * @return: SCU_SUCCESS on success, or the first error encountered in the
 * pipeline
 */
scu_result cstate_compile(cstate *cst);

/*
 * @brief: Free all associated memory of a compiler state
 *
 * @param s: pointer to malloc'd cstate.
 */
void cstate_free(cstate *s);

CREG_CLEANUP_SHIM(cstate, cstate_free)

#endif // !CSTATE_H
