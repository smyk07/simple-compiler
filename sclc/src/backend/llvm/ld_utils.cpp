/*
 * ld_utils: calls linker
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

extern "C" {
#include "core/utils.h"
}

#include "sclc/backend/llvm/ld_utils.hpp"

#include <cstdio>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

scu_result ld_link(const char *output_file,
                   const std::vector<const char *> &obj_files) {
  std::vector<const char *> args;

  args.push_back("cc");

  args.push_back("-o");
  args.push_back(output_file);

  for (const char *obj : obj_files) {
    args.push_back(obj);
  }

  args.push_back(nullptr);

  pid_t pid = fork();
  if (pid < 0) {
    scu_perror((char *)"fork failed\n");
    return SCU_ERR_LINK;
  }

  if (pid == 0) {
    execvp("cc", const_cast<char *const *>(args.data()));
    perror("execvp cc");
    _exit(1);
  }

  int status = 0;
  waitpid(pid, &status, 0);

  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    scu_perror((char *)"Linking failed\n");
    return SCU_ERR_LINK;
  }

  return SCU_SUCCESS;
}
