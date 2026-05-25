/*
 * ld_utils: calls linker
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#ifndef LD_UTILS_H
#define LD_UTILS_H

#include "core/utils.h"

#include <vector>

/*
 * @brief: helper function to link the generated output object file to an
 * executable binary.
 *
 * @param output_file: name to be given to the output executable binary.
 * @param obj_files: vector of object files to be linked.
 *
 * @return: SCU_SUCCESS on success, or SCU_ERR_LINK
 */
scu_result ld_link(const char *output_file,
                   const std::vector<const char *> &obj_files);

#endif // !LD_UTILS_H
