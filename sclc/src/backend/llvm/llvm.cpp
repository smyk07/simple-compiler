/*
 * llvm: acts as a link between sclc frontend and the llvm backend.
 *
 * Scull Project Copyright (C) 2026, Samyak Bambole <bambole@duck.com>
 * Licensed under the GNU/GPL Version 3
 */

#include "backend/llvm/llvm.h"
#include "backend/llvm/ld_utils.hpp"
#include "backend/llvm/llvm_irgen.hpp"
#include <filesystem>
#include <stddef.h>

extern "C" {
#include "ast.h"
#include "backend/llvm/llvm.h"
#include "common.h"
#include "cstate.h"
#include "ds/dynamic_array.h"
#include "fstate.h"
#include "utils.h"
}

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

typedef struct llvm_backend_ctx llvm_backend_ctx;

static llvm_backend_ctx bctx;

extern "C" {
void llvm_backend_init(cstate *cst) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmParser();
  llvm::InitializeNativeTargetAsmPrinter();

  bctx.context = new llvm::LLVMContext();

  std::string module_name = cst->output_filepath;
  bctx.module = new llvm::Module(module_name, *bctx.context);

  bctx.builder = new llvm::IRBuilder<>(*bctx.context);

  bctx.target_triple = cst->llvm_target_triple;

  bctx.module->setTargetTriple(bctx.target_triple);

  std::string error;
  const llvm::Target *target =
      llvm::TargetRegistry::lookupTarget(bctx.target_triple, error);

  if (!target) {
    scu_perror(const_cast<char *>("Failed to look up target: %s\n"),
               error.c_str());
    return;
  }

  llvm::TargetOptions opt;
  llvm::Reloc::Model RM = llvm::Reloc::PIC_;

  bctx.target_machine = target->createTargetMachine(
      bctx.target_triple, "generic", "", opt, RM, llvm::CodeModel::Small);

  if (!bctx.target_machine) {
    scu_perror(const_cast<char *>("Failed to create target machine\n"));
    return;
  }

  bctx.module->setDataLayout(bctx.target_machine->createDataLayout());
}

void llvm_backend_compile(cstate *, fstate *fst) {
  for (u64 i = 0; i < fst->program_ast.instrs.count; i++) {
    instr_node instr;
    dynamic_array_get(&fst->program_ast.instrs, i, &instr);

    llvm_irgen_instr(bctx, &instr);
  }
  llvm_irgen_clear_symbol_table();
}

void llvm_backend_optimize(cstate *cst, fstate *) {
  using namespace llvm;

  OptimizationLevel opt_level;

  switch (cst->options.opt_level) {
  case OPT_O0:
    opt_level = OptimizationLevel::O0;
    break;
  case OPT_O1:
    opt_level = OptimizationLevel::O1;
    break;
  case OPT_O2:
    opt_level = OptimizationLevel::O2;
    break;
  case OPT_O3:
    opt_level = OptimizationLevel::O3;
    break;
  case OPT_Os:
    opt_level = OptimizationLevel::Os;
    break;
  case OPT_Oz:
    opt_level = OptimizationLevel::Oz;
    break;
  }

  if (opt_level == OptimizationLevel::O0)
    return;

  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;

  PassBuilder PB;

  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);

  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(opt_level);

  MPM.run(*bctx.module, MAM);
}

void llvm_backend_emit(cstate *cst, fstate *fst) {
  std::string error_str;
  std::error_code ec;
  llvm::raw_string_ostream error_stream(error_str);

  if (llvm::verifyModule(*bctx.module, &error_stream)) {
    error_stream.flush();
    scu_perror(const_cast<char *>("Module verification failed: %s\n"),
               error_str.c_str());
    return;
  }

  if (cst->options.emit_llvm) {
    std::string ir_filename = std::string(fst->extracted_filepath) + ".ll";
    llvm::raw_fd_ostream ir_file(ir_filename, ec, llvm::sys::fs::OF_None);

    if (!ec) {
      bctx.module->print(ir_file, nullptr);
      ir_file.close();
    } else {
      scu_pwarning(const_cast<char *>("Could not write IR file: %s\n"),
                   ec.message().c_str());
    }

    return;
  }

  if (cst->options.emit_asm) {
    std::string asm_filename = std::string(fst->extracted_filepath) + ".s";
    llvm::raw_fd_ostream asm_dest(asm_filename, ec, llvm::sys::fs::OF_None);

    if (ec) {
      scu_pwarning(const_cast<char *>("Could not open asm file: %s\n"),
                   ec.message().c_str());
      return;
    }

    llvm::legacy::PassManager asm_pass;
    if (bctx.target_machine->addPassesToEmitFile(
            asm_pass, asm_dest, nullptr, llvm::CodeGenFileType::AssemblyFile)) {
      scu_pwarning(const_cast<char *>("TargetMachine can't emit assembly\n"));
    } else {
      asm_pass.run(*bctx.module);
      asm_dest.flush();
    }

    return;
  }

  std::string obj_filename;

  if (cst->options.compile_only)
    obj_filename = std::string(fst->extracted_filepath) + ".o";
  else {
    obj_filename = "/tmp/sclc/" + std::string(fst->extracted_filepath) + ".o";

    std::filesystem::path obj_path(obj_filename);
    std::filesystem::path parent_dir = obj_path.parent_path();

    if (!parent_dir.empty()) {
      std::error_code mkdir_ec;
      std::filesystem::create_directories(parent_dir, mkdir_ec);

      if (mkdir_ec) {
        scu_perror(
            const_cast<char *>("Could not create temporary directory: %s\n"),
            mkdir_ec.message().c_str());
        return;
      }
    }
  }

  llvm::raw_fd_ostream dest(obj_filename, ec, llvm::sys::fs::OF_None);

  if (ec) {
    scu_perror(const_cast<char *>("Could not open output file: %s\n"),
               ec.message().c_str());
    return;
  }

  llvm::legacy::PassManager pass;

  if (bctx.target_machine->addPassesToEmitFile(
          pass, dest, nullptr, llvm::CodeGenFileType::ObjectFile)) {
    scu_perror(const_cast<char *>("TargetMachine can't emit object file\n"));
    return;
  }

  pass.run(*bctx.module);
  dest.flush();
}

void llvm_backend_cleanup(cstate *, fstate *) {
  delete bctx.builder;
  delete bctx.module;
  delete bctx.context;

  if (bctx.target_machine) {
    delete bctx.target_machine;
  }

  bctx.builder = nullptr;
  bctx.module = nullptr;
  bctx.context = nullptr;
  bctx.target_machine = nullptr;
}

void llvm_backend_link(cstate *cst) {
  std::vector<const char *> obj_files;

  for (u64 i = 0; i < cst->obj_file_list.count; i++) {
    char *obj;
    dynamic_array_get(&cst->obj_file_list, i, &obj);
    obj_files.push_back(obj);
  }

  ld_link(cst->output_filepath, obj_files);
}
}
