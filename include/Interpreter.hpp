#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "general.hpp"

class Interpreter : public Visitor {
public:
  using PossibleValue = ::PossibleValue;

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
  void Visit(const Program &node) override;
  void Visit(const NewArrayExpression &node) override;
  void Visit(const ArrayIndexExpression &node) override;
  void Visit(const ClassDeclaration &node) override;
  void Visit(const MethodDeclaration &) override;
  void Visit(const NewObjectExpression &node) override;
  void Visit(const MethodCallExpression &node) override;
  void Visit(const MethodCallStatement &node) override;
  void Visit(const FieldAccessExpression &node) override;
  void Visit(const FunctionDeclaration &node) override;
  void Visit(const FunctionCallExpression &node) override;
  void Visit(const FunctionCallStatement &node) override;

  PossibleValue &GetLastValue();
  [[nodiscard]] PossibleValue GetLastValue() const;

  std::unordered_map<std::string, PossibleValue> &GetVariables();
  [[nodiscard]] std::unordered_map<std::string, PossibleValue>
  GetVariables() const;

  PossibleValue &GetVar(const std::string &name);
  [[nodiscard]] PossibleValue GetVar(const std::string &name) const;

private:
  RuntimeData runtime_data_;
  std::unordered_map<std::string, PossibleValue> variables_;
  PossibleValue last_value_;

  struct ClassData {
    const std::vector<FieldDeclaration> *fields;
    std::unordered_map<std::string, const MethodDeclaration *> methods;

    explicit ClassData(const ClassDeclaration &node);
  };

  std::unordered_map<std::string, ClassData> classes_;
  std::unordered_map<std::string, const FunctionDeclaration *> functions_;

  static int AsInt(const PossibleValue &v);
  static bool AsBool(const PossibleValue &v);
  ArrayData &AsArray(const PossibleValue &v);
  ObjectData &AsObject(const PossibleValue &v);

  PossibleValue
  CallFunction(const std::string &function_name,
               const std::vector<std::unique_ptr<Expression>> &arg_nodes);

  PossibleValue
  CallMethod(const std::string &object_name, const std::string &method_name,
             const std::vector<std::unique_ptr<Expression>> &arg_nodes);
};