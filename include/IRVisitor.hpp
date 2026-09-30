#pragma once

#include "ScopeVisitor.hpp"
#include "tree.hpp"
#include "types.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <string>
#include <unordered_map>

class IRVisitor : public Visitor {
public:
  explicit IRVisitor(const SymbolTable &symbol_table);

  void Visit(const IntLiteral &) override;
  void Visit(const BoolLiteral &) override;
  void Visit(const IdentityLiteral &) override;
  void Visit(const UnaryOperation &) override;
  void Visit(const BinaryOperation &) override;
  void Visit(const VariableDeclaration &) override;
  void Visit(const AssignStatement &) override;
  void Visit(const ArrayAssignStatement &) override;
  void Visit(const IfStatement &) override;
  void Visit(const IfElseStatement &) override;
  void Visit(const WhileStatement &) override;
  void Visit(const PrintStatement &) override;
  void Visit(const StructDeclaration &) override;
  void Visit(const MethodDeclaration &) override;
  void Visit(const NewObjectExpression &) override;
  void Visit(const NewArrayExpression &) override;
  void Visit(const ArrayIndexExpression &) override;
  void Visit(const MethodCallExpression &) override;
  void Visit(const MethodCallStatement &) override;
  void Visit(const FieldAccessExpression &) override;
  void Visit(const FunctionDeclaration &) override;
  void Visit(const FunctionCallExpression &) override;
  void Visit(const FunctionCallStatement &) override;
  void Visit(const ReturnStatement &) override;
  void Visit(const Program &) override;

  void Print(llvm::raw_ostream &os) const;
  bool SaveToFile(const std::string &path) const;

  std::unique_ptr<llvm::Module> GetModule();

private:
  struct LocalVar {
    llvm::AllocaInst *alloca{};
    Type type;
  };

  struct FieldPtr {
    llvm::Value *value{};
    Type type;
  };

  llvm::LLVMContext context_;
  std::unique_ptr<llvm::Module> module_;
  llvm::IRBuilder<> builder_;
  const SymbolTable &symbol_table_;
  llvm::Value *last_value_ = nullptr;
  llvm::Function *current_function_ = nullptr;
  std::unordered_map<std::string, LocalVar> local_vars_;
  std::unordered_map<std::string, FieldPtr> fields_;
  std::unordered_map<std::string, llvm::StructType *> struct_types_;
  std::unordered_map<std::string, std::unordered_map<std::string, unsigned>>
      field_indices_;
  std::unordered_map<std::string, std::unordered_map<std::string, Type>>
      field_types_;
  std::string current_struct_;
  llvm::Value *self_ptr_ = nullptr;
  llvm::FunctionCallee printf_function_;
  llvm::FunctionCallee malloc_function_;
  llvm::Value *fmt_int_ = nullptr;

  void DeclareExternals();
  void DeclareStructTypes(const Program &);
  void DeclareFunctionSignatures(const Program &);
  llvm::Type *LLVMType(const Type &);
  static std::string MethodFunctionName(const std::string &strct,
                                        const std::string &method);
  llvm::AllocaInst *CreateEntryAlloca(const std::string &name,
                                      llvm::Type *ty) const;
  std::pair<llvm::Value *, Type> LookupAddress(const std::string &name);
  void EmitBody(const Statements &);
  llvm::Value *SizeOf(llvm::Type *ty);
  llvm::Value *EmitMalloc(llvm::Value *byte_count);
  void AddDefaultReturn(const Type &return_type);
  bool IsBlockTerminated() const;

  llvm::Type *i32Ty() { return llvm::Type::getInt32Ty(context_); }

  llvm::Type *i1Ty() { return llvm::Type::getInt1Ty(context_); }

  llvm::Type *i64Ty() { return llvm::Type::getInt64Ty(context_); }

  llvm::Type *ptrTy() { return llvm::PointerType::getUnqual(context_); }

  llvm::Type *voidTy() { return llvm::Type::getVoidTy(context_); }
};