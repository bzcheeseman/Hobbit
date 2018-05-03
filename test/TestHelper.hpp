//
// Created by Aman on 5/3/18.
//

#ifndef HOBBIT_TESTHELPER_HPP
#define HOBBIT_TESTHELPER_HPP

#include <llvm/IR/Module.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

std::unique_ptr<llvm::Module> CreateLLVMModule(llvm::LLVMContext &ctx, const std::string &module_name) {
  return llvm::make_unique<llvm::Module>(module_name, ctx);
}

template<class... Args>
llvm::Function *CreateLLVMFunction(
        std::unique_ptr<llvm::Module> &module, const std::string &name, llvm::Type *ret_ty, Args... args
) {
  llvm::Function *func = llvm::cast<llvm::Function>(
          module->getOrInsertFunction(name, ret_ty, args...)
  );
  return func;
}

void FinalizeModule(std::unique_ptr<llvm::Module> &module,
                    unsigned int opt_level,
                    const std::string &target_triple,
                    const std::string &cpu,
                    const std::string &features) {
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  module->setTargetTriple(target_triple);

  std::string error;
  auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);

  llvm::TargetOptions options;
  auto RM = llvm::Optional<llvm::Reloc::Model>();

  llvm::TargetMachine *target_machine =
          target->createTargetMachine(target_triple, cpu, features, options, RM);

  module->setDataLayout(target_machine->createDataLayout());
  module->setTargetTriple(target_triple);

  llvm::verifyModule(*module, &llvm::errs());

  llvm::legacy::PassManager pm;
  llvm::PassManagerBuilder PMBuilder;

  PMBuilder.OptLevel = opt_level;

  PMBuilder.populateModulePassManager(pm);
  target_machine->adjustPassManager(PMBuilder);

  pm.run(*module);

  llvm::verifyModule(*module, &llvm::errs());
}

#endif //HOBBIT_TESTHELPER_HPP
