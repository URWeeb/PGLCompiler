#include "Compiler.hpp"

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/CodeGen.h>
#include <cstdlib>
#include <memory>
#include <string>
#include <optional>

bool Compiler::CompileToExecutable(const std::unique_ptr<llvm::Module> &module, const std::string &file_name) {
    static bool initialized = false;

    if (!initialized) {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();
        initialized = true;
    }

    std::string target_triple = module->getTargetTriple();

    if (target_triple.empty()) {
        target_triple = llvm::sys::getDefaultTargetTriple();
        module->setTargetTriple(target_triple);
    }

    std::string error;
    const llvm::Target *target = llvm::TargetRegistry::lookupTarget(target_triple, error);

    if (!target) {
        llvm::errs() << "Target lookup error: " << error << "\n";
        return false;
    }

    llvm::TargetOptions options;
    std::optional<llvm::Reloc::Model> reloc_model = llvm::Reloc::Model::PIC_;
    std::string cpu = llvm::sys::getHostCPUName().str();
    std::string features;
    auto host_features = llvm::sys::getHostCPUFeatures();

    for (const auto& feature : host_features) {
        features += (feature.second ? "+" : "-") + feature.first().str() + ",";
    }

    std::unique_ptr<llvm::TargetMachine> target_machine(target->createTargetMachine(target_triple, cpu, features, options, reloc_model));
    module->setDataLayout(target_machine->createDataLayout());

    llvm::LoopAnalysisManager loop_analysis_manager;
    llvm::FunctionAnalysisManager func_analysis_manager;
    llvm::CGSCCAnalysisManager cgs_analysis_manager;
    llvm::ModuleAnalysisManager module_analysis_manager;
    llvm::PassBuilder pass_builder;

    pass_builder.registerModuleAnalyses(module_analysis_manager);
    pass_builder.registerCGSCCAnalyses(cgs_analysis_manager);
    pass_builder.registerFunctionAnalyses(func_analysis_manager);
    pass_builder.registerLoopAnalyses(loop_analysis_manager);
    pass_builder.crossRegisterProxies(loop_analysis_manager, func_analysis_manager, cgs_analysis_manager, module_analysis_manager);

    llvm::ModulePassManager module_pass_manager = pass_builder.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);
    module_pass_manager.run(*module, module_analysis_manager);

    std::string object_filename = file_name + ".o";
    std::error_code error_code;
    llvm::raw_fd_ostream dest(object_filename, error_code, llvm::sys::fs::OF_None);

    if (error_code) {
        llvm::errs() << "Cannot open output file: " << error_code.message() << "\n";
        return false;
    }

    llvm::legacy::PassManager pass_manager;

    if (target_machine->addPassesToEmitFile(pass_manager, dest, nullptr, llvm::CodeGenFileType::ObjectFile)) {
        llvm::errs() << "TargetMachine cannot emit object file\n";
        return false;
    }

    pass_manager.run(*module);
    dest.flush();
    dest.close();

    auto linking_tryer = [&](const std::string& command) {
        return std::system((command + object_filename + " -o " + file_name).c_str());
    };

    int return_value = linking_tryer("clang-20 ");

    if (return_value != 0) {
        return_value = linking_tryer("clang ");
    }

    if (return_value != 0) {
        llvm::errs() << "Linking failed\n";
        return false;
    }

    return true;
}
