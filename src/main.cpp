#include "IRVisitor.hpp"
#include "Compiler.hpp"
#include "parser.tab.hh"
#include "tree.hpp"
#include "visitors.hpp"
#include <exception>
#include <string>

int main(int argc, char *argv[]) {
  std::string output_file;
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "-o" && i + 1 < argc) {
      output_file = argv[++i];
    }
  }

  Program *root = nullptr;
  yy::parser parser(root);
  parser.parse();

  if (root) {
    PrintVisitor printer("tree.txt");
    root->Accept(printer);

    ScopeVisitor scope;

    try {
      root->Accept(scope);
    } catch (const std::exception &e) {
      fprintf(stderr, "Semantic error: %s\n", e.what());
      delete root;
      return 1;
    }

    TypeChecker type_checker(scope.GetSymbolTable());

    try {
      root->Accept(type_checker);
    } catch (const std::exception &e) {
      fprintf(stderr, "Type error: %s\n", e.what());
      delete root;
      return 1;
    }

    if (!output_file.empty()) {
      IRVisitor ir(scope.GetSymbolTable());
      root->Accept(ir);

      if (!Compiler::CompileToExecutable(ir.GetModule(), output_file)) {
        fprintf(stderr, "Compilation error\n");
        delete root;
        return 1;
      }

      printf("Successful compilation to %s.\n", output_file.c_str());
    } else {
      Interpreter interpreter;
      root->Accept(interpreter);
    }
  }

  delete root;
  return 0;
}