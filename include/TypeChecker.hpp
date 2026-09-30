#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "ScopeVisitor.hpp"
#include "tree.hpp"
#include "types.hpp"

class TypeChecker : public Visitor {
public:
  explicit TypeChecker(const SymbolTable &symbol_table);
  ~TypeChecker() override = default;

  void Visit(const IntLiteral &) override;
  void Visit(const BoolLiteral &) override;
  void Visit(const IdentityLiteral &node) override;
  void Visit(const UnaryOperation &node) override;
  void Visit(const BinaryOperation &node) override;
  void Visit(const VariableDeclaration &node) override;
  void Visit(const AssignStatement &node) override;
  void Visit(const ArrayAssignStatement &node) override;
  void Visit(const IfStatement &node) override;
  void Visit(const IfElseStatement &node) override;
  void Visit(const WhileStatement &node) override;
  void Visit(const PrintStatement &node) override;
  void Visit(const StructDeclaration &node) override;
  void Visit(const MethodDeclaration &node) override;
  void Visit(const NewObjectExpression &node) override;
  void Visit(const NewArrayExpression &node) override;
  void Visit(const ArrayIndexExpression &node) override;
  void Visit(const MethodCallExpression &node) override;
  void Visit(const MethodCallStatement &node) override;
  void Visit(const FieldAccessExpression &node) override;
  void Visit(const FunctionDeclaration &node) override;
  void Visit(const FunctionCallExpression &node) override;
  void Visit(const FunctionCallStatement &node) override;
  void Visit(const ReturnStatement &node) override;
  void Visit(const Program &node) override;

private:
  const SymbolTable &symbol_table_;
  Type current_type_ = VoidType{};
  Type expected_return_type_ = VoidType{};
  std::string current_struct_;
  std::vector<std::unordered_map<std::string, Type>> scopes_;

  void PushScope();
  void PopScope();
  void DeclareVariable(const std::string &name, const Type &type);
  [[nodiscard]] Type GetVariableType(const std::string &name) const;

  static void ExpectType(const Type &actual, const Type &expected,
                         const std::string &context);

  void CheckMethodCall(const std::string &object_name,
                       const std::string &method_name,
                       const std::vector<std::unique_ptr<Expression>> &args);

  void CheckFunctionCall(const std::string &function_name,
                         const std::vector<std::unique_ptr<Expression>> &args);
};
