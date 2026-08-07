#pragma once

#include <fstream>
#include <string>

#include "tree.hpp"

class PrintVisitor : public Visitor {
public:
  explicit PrintVisitor(const std::string &filepath);

  void Visit(const IntLiteral &node) override;
  void Visit(const BoolLiteral &node) override;
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
  void Visit(const ReturnStatement &node) override;
  void Visit(const ClassDeclaration &node) override;
  void Visit(const MethodDeclaration &node) override;
  void Visit(const FunctionDeclaration &node) override;
  void Visit(const NewObjectExpression &node) override;
  void Visit(const NewArrayExpression &node) override;
  void Visit(const ArrayIndexExpression &node) override;
  void Visit(const MethodCallExpression &node) override;
  void Visit(const MethodCallStatement &node) override;
  void Visit(const FunctionCallExpression &node) override;
  void Visit(const FunctionCallStatement &node) override;
  void Visit(const FieldAccessExpression &node) override;
  void Visit(const Program &node) override;

private:
  std::ofstream output_;
  size_t indent_ = 0;

  void Write(const std::string &str);
};
