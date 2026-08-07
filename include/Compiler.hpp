#pragma once

#include <llvm/IR/Module.h>
#include <string>

class Compiler {
public:
    static bool CompileToExecutable(const std::unique_ptr<llvm::Module> &module, const std::string &file_name);
};