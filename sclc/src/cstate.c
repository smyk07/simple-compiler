/*
 * cstate: Per-binary compilation state, groups all variables and options for
 * one build unit.
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#include "sclc/cstate.h"
#include "sclc/backend/backend.h"
#include "sclc/fstate.h"

#include "core/common.h"
#include "core/ds/arena.h"
#include "core/ds/dynamic_array.h"
#include "core/utils.h"

#include <llvm-c/Core.h>
#include <llvm-c/TargetMachine.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void cstate_init(cstate *cst, u32 argc, char *argv[]) {
  if (argc <= 1) {
    /*
     * Default output.
     * Should list out each option and a short description.
     * Take care of wrapping after ~80 characters.
     */

  display_help_prompt:
    printf("SCULL Compiler\n");
    printf("Usage: %s [OPTIONS] <input_files>\n\n", argv[0]);

    printf("OPTIONS:\n");

    printf("--target [TARGET]                     Specify LLVM supported "
           "output target triple\n");

    printf("-c                                    Compile but do not link\n");

    printf("--output <output_filename>    OR  -o  Specify output filename.\n");

    printf("--include_dir <path_to_dir>   OR  -i  Specify include directory "
           "path.\n");

    printf("--verbose                     OR  -v  Print debug messages.\n");

    printf("--emit-llvm                           Emit LLVM IR along with "
           "object file.\n");

    printf("--emit-asm                            Emit target assembly along "
           "with object file.\n");

    printf("-O0, -O1, -O2, -O3, -Os, -Oz          Optimization levels\n");

    printf("\n");

    printf("--help                        OR  -h  Display this help prompt\n");

    /*
    printf("Available Targets:\n");
    for (int i = 0; i <= TARGET_C; i++) {
      printf("%s ", target_kind_to_string((target_kind)i));
    }
    printf("\n");
    */

    exit(0);
  }

  u32 i = 1;

  dynamic_array filenames;
  dynamic_array_init(&filenames, sizeof(char *));

  dynamic_array_init(&cst->obj_file_list, sizeof(char *));
  cst->output_filepath = NULL;
  cst->include_dir = NULL;
  cst->llvm_target_triple = LLVMGetDefaultTargetTriple();
  cst->options.opt_level = OPT_O2;

  while (i < argc) {
    char *arg = argv[i];

    if (strcmp(arg, "--target") == 0) {
      LLVMInitializeAllTargetInfos();

      if (i + 1 >= argc) {
        scu_perror("Missing target after %s\n", arg);
        exit(1);
      }

      char *target_str = argv[i + 1];
      char *err = NULL;
      LLVMTargetRef T = NULL;

      if (LLVMGetTargetFromTriple(target_str, &T, &err) != 0) {
        scu_perror("Invalid or unsupported target triple '%s': %s\n",
                   target_str, err ? err : "unknown error");

        if (err)
          LLVMDisposeMessage(err);
        exit(1);
      }

      cst->options.target_specified = true;

      if (cst->llvm_target_triple)
        free(cst->llvm_target_triple);

      cst->llvm_target_triple = strdup(target_str);

      i += 2;
      continue;
    }

    if (strcmp(arg, "--output") == 0 || strcmp(arg, "-o") == 0) {
      if (i + 1 >= argc) {
        scu_perror("Missing filename after %s\n", arg);
        exit(1);
      }

      if (cst->output_filepath != NULL) {
        scu_perror("Output specified more than once: %s\n", argv[i + 1]);
        exit(1);
      }

      cst->output_filepath = strdup(argv[i + 1]);
      cst->options.explicit_output_specified = true;
      i += 2;
      continue;
    }

    if (strcmp(arg, "-c") == 0) {
      cst->options.compile_only = true;
      i++;
      continue;
    }

    if (strcmp(arg, "--include_dir") == 0 || strcmp(arg, "-i") == 0) {
      if (i + 1 >= argc) {
        scu_perror("Missing directory path after %s\n", arg);
        exit(1);
      }

      if (cst->include_dir != NULL) {
        scu_perror("Include directory specified more than once: %s\n",
                   argv[i + 1]);
        exit(1);
      }

      struct stat st;
      if (stat(argv[i + 1], &st) != 0) {
        scu_perror("Include directory does not exist: %s\n", argv[i + 1]);
        exit(1);
      }

      if (!S_ISDIR(st.st_mode)) {
        scu_perror("Path is not a directory: %s\n", argv[i + 1]);
        exit(1);
      }

      cst->include_dir = strdup(argv[i + 1]);
      cst->options.include_dir_specified = true;

      i += 2;
      continue;
    }

    if (strcmp(arg, "--verbose") == 0 || strcmp(arg, "-v") == 0) {
      cst->options.verbose = true;
      i++;
      continue;
    }

    if (strcmp(arg, "--emit-llvm") == 0) {
      cst->options.emit_llvm = true;
      cst->options.compile_only = true;
      i++;
      continue;
    }

    if (strcmp(arg, "--emit-asm") == 0) {
      cst->options.emit_asm = true;
      cst->options.compile_only = true;
      i++;
      continue;
    }

    if (arg[0] != '-') {
      char *filename_copy = strdup(arg);
      dynamic_array_append(&filenames, &filename_copy);
      i++;
      continue;
    }

    if (strcmp(arg, "-O0") == 0) {
      cst->options.opt_level = OPT_O0;
      i++;
      continue;
    }

    if (strcmp(arg, "-O1") == 0) {
      cst->options.opt_level = OPT_O1;
      i++;
      continue;
    }

    if (strcmp(arg, "-O2") == 0) {
      cst->options.opt_level = OPT_O2;
      i++;
      continue;
    }

    if (strcmp(arg, "-O3") == 0) {
      cst->options.opt_level = OPT_O3;
      i++;
      continue;
    }

    if (strcmp(arg, "-Os") == 0) {
      cst->options.opt_level = OPT_Os;
      i++;
      continue;
    }

    if (strcmp(arg, "-Oz") == 0) {
      cst->options.opt_level = OPT_Oz;
      i++;
      continue;
    }

    if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
      goto display_help_prompt;
    }

    scu_perror("Unknown option: %s\n", arg);
    exit(1);
  }

  if (cst->include_dir == NULL)
    cst->include_dir = strdup(".");

  if (filenames.count == 0) {
    scu_perror("Missing input filename\n");
    exit(1);
  }

  bool compile_mode = cst->options.compile_only || cst->options.emit_llvm ||
                      cst->options.emit_asm;

  if (compile_mode && cst->output_filepath != NULL && filenames.count > 1) {
    scu_perror("-o cannot be used with multiple input files in compile mode. "
               "Compile each file separately or remove -o\n");
    exit(1);
  }

  if (cst->output_filepath == NULL) {
    char *first_filename;
    dynamic_array_get(&filenames, 0, &first_filename);
    cst->output_filepath = scu_extract_name(first_filename);
    if (!cst->output_filepath) {
      scu_perror("Failed to extract filename.\n");
      exit(1);
    }
  }

  arena_init(&cst->file_arena);
  dynamic_array_init(&cst->files, sizeof(fstate *));

  for (u64 i = 0; i < filenames.count; i++) {
    char *filepath;
    dynamic_array_get(&filenames, i, &filepath);

    fstate *fst = arena_push_struct(&cst->file_arena, fstate);
    fstate_init(fst, filepath);
    dynamic_array_append(&cst->files, &fst);

    u64 len;
    char *obj;

    if (compile_mode) {
      if (cst->output_filepath != NULL) {
        obj = strdup(cst->output_filepath);
      } else {
        len = strlen(fst->extracted_filepath) + 3;
        obj = scu_checked_malloc(len);
        snprintf(obj, len, "%s.o", fst->extracted_filepath);
      }
    } else {
      len = strlen(fst->extracted_filepath) + 13;
      obj = scu_checked_malloc(len);
      snprintf(obj, len, "/tmp/sclc/%s.o", fst->extracted_filepath);
    }

    dynamic_array_append(&cst->obj_file_list, &obj);

    free(filepath);
  }

  dynamic_array_free(&filenames);
}

void cstate_free(cstate *cst) {
  if (cst == NULL)
    return;

  if (cst->include_dir != NULL)
    free(cst->include_dir);

  if (cst->output_filepath != NULL)
    free(cst->output_filepath);

  if (cst->llvm_target_triple != NULL)
    free(cst->llvm_target_triple);

  for (u64 i = 0; i < cst->files.count; i++) {
    fstate *fst;
    dynamic_array_get(&cst->files, i, &fst);
    fstate_free(fst);
  }

  arena_free(&cst->file_arena);

  for (u64 i = 0; i < cst->obj_file_list.count; i++) {
    char *objfname;
    dynamic_array_get(&cst->obj_file_list, i, &objfname);
    free(objfname);
  }

  dynamic_array_free(&cst->obj_file_list);

  dynamic_array_free(&cst->files);
}
