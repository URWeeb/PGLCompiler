#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "tree.hpp"
#include "types.hpp"

struct CallableInfo {
  std::string name;
  std::vector<Parameter> params;
  Type return_type;

  CallableInfo() = default;

  template <typename T>
  CallableInfo(const T &node)
    requires(std::is_same_v<T, FunctionDeclaration> ||
             std::is_same_v<T, MethodDeclaration>)
      : name(node.name), params(node.parameters),
        return_type(node.return_type) {}
};

struct StructInfo {
  std::string name;
  std::unordered_map<std::string, Type> fields;
  std::unordered_map<std::string, CallableInfo> methods;

  StructInfo() = default;

  explicit StructInfo(const StructDeclaration &node);

  [[nodiscard]] const CallableInfo *
  FindMethod(const std::string &method_name) const;
  [[nodiscard]] const Type *FindField(const std::string &field_name) const;
};

class SymbolTable {
public:
  void AddStruct(StructInfo info) {
    structs_.insert_or_assign(info.name, std::move(info));
  }

  [[nodiscard]] const StructInfo *GetStruct(const std::string &name) const {
    auto it = structs_.find(name);
    return it != structs_.end() ? &it->second : nullptr;
  }

  [[nodiscard]] bool HasStruct(const std::string &name) const {
    return structs_.contains(name);
  }

  void AddFunction(CallableInfo info) {
    functions_.insert_or_assign(info.name, std::move(info));
  }

  [[nodiscard]] const CallableInfo *GetFunction(const std::string &name) const {
    auto it = functions_.find(name);
    return it != functions_.end() ? &it->second : nullptr;
  }

  [[nodiscard]] bool HasFunction(const std::string &name) const {
    return functions_.contains(name);
  }

private:
  std::unordered_map<std::string, StructInfo> structs_;
  std::unordered_map<std::string, CallableInfo> functions_;
};

struct VariableInfo {
  std::string name;
  Type type;

  VariableInfo() = default;

  template <typename T>
  explicit VariableInfo(const T &node)
    requires(std::is_base_of_v<StorageDeclaration, T>)
      : name(node.name), type(node.type) {}
};

struct ScopeNode {
  ScopeNode *parent = nullptr;
  std::vector<std::unique_ptr<ScopeNode>> children;
  std::unordered_map<std::string, VariableInfo> variables;

  ScopeNode *AddChild();
  [[nodiscard]] const VariableInfo *Lookup(const std::string &name) const;
  bool DeclareLocal(const std::string &name, const VariableInfo& info);
};

class ScopeVisitor : public Visitor {
public:
  ScopeVisitor();

  void Visit(const StructDeclaration &node) override;
  void Visit(const MethodDeclaration &node) override;
  void Visit(const IntLiteral &) override;
  void Visit(const BoolLiteral &) override;
  void Visit(const IdentityLiteral &node) override;
  void Visit(const UnaryOperation &node) override;
  void Visit(const BinaryOperation &node) override;
  void Visit(const NewObjectExpression &node) override;
  void Visit(const NewArrayExpression &node) override;
  void Visit(const ArrayIndexExpression &node) override;
  void Visit(const ArrayAssignStatement &node) override;
  void Visit(const MethodCallExpression &node) override;
  void Visit(const MethodCallStatement &node) override;
  void Visit(const FieldAccessExpression &node) override;
  void Visit(const VariableDeclaration &node) override;
  void Visit(const AssignStatement &node) override;
  void Visit(const IfStatement &node) override;
  void Visit(const IfElseStatement &node) override;
  void Visit(const WhileStatement &node) override;
  void Visit(const PrintStatement &node) override;
  void Visit(const FunctionDeclaration &node) override;
  void Visit(const ReturnStatement &node) override;
  void Visit(const FunctionCallExpression &node) override;
  void Visit(const FunctionCallStatement &node) override;
  void Visit(const Program &node) override;

  ScopeNode *GetRoot() const;
  [[nodiscard]] const SymbolTable &GetSymbolTable() const;

private:
  std::unique_ptr<ScopeNode> root_;
  ScopeNode *current_;
  SymbolTable symbol_table_;
  std::string current_struct_;
  bool inside_callable_ = false;

  void PushScope();
  void PopScope();

  const VariableInfo *RequireDeclared(const std::string &name) const;
  void RequireValidType(const Type &type, const std::string &context);
};
