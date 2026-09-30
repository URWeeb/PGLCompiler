#include "ScopeVisitor.hpp"

StructInfo::StructInfo(const StructDeclaration &node) : name(node.name) {
  for (auto &field : node.fields) {
    fields[field.name] = field.type;
  }

  for (auto &method : node.methods) {
    methods.insert_or_assign(method->name, CallableInfo(*method));
  }
}

const CallableInfo *
StructInfo::FindMethod(const std::string &method_name) const {
  auto iter = methods.find(method_name);

  return (iter != methods.end() ? &iter->second : nullptr);
}

const Type *StructInfo::FindField(const std::string &field_name) const {
  const auto iter = fields.find(field_name);

  return (iter != fields.end() ? &iter->second : nullptr);
}

ScopeNode *ScopeNode::AddChild() {
  children.push_back(std::make_unique<ScopeNode>());
  children.back()->parent = this;
  return children.back().get();
}

const VariableInfo *ScopeNode::Lookup(const std::string &name) const {
  const auto it = variables.find(name);

  if (it != variables.end()) {
    return &it->second;
  }

  if (parent) {
    return parent->Lookup(name);
  }

  return nullptr;
}

bool ScopeNode::DeclareLocal(const std::string &name,
                             const VariableInfo &info) {
  return variables.try_emplace(name, info).second;
}

ScopeVisitor::ScopeVisitor() {
  root_ = std::make_unique<ScopeNode>();
  current_ = root_.get();
}

ScopeNode *ScopeVisitor::GetRoot() const { return root_.get(); }

const SymbolTable &ScopeVisitor::GetSymbolTable() const {
  return symbol_table_;
}

void ScopeVisitor::PushScope() { current_ = current_->AddChild(); }
void ScopeVisitor::PopScope() { current_ = current_->parent; }

const VariableInfo *
ScopeVisitor::RequireDeclared(const std::string &name) const {
  const VariableInfo *info = current_->Lookup(name);

  if (!info) {
    throw std::runtime_error("Variable '" + name +
                             "' is used but was never declared");
  }

  return info;
}

void ScopeVisitor::RequireValidType(const Type &type,
                                    const std::string &context) {
  std::visit(overloaded{[](const IntType &) {}, [](const BoolType &) {},
                        [](const VoidType &) {},
                        [&](const StructType &c) {
                          if (!symbol_table_.HasStruct(c.name)) {
                            throw std::runtime_error("Unknown struct '" +
                                                     c.name + "' in '" +
                                                     context + "'");
                          }
                        },
                        [&](const ArrayType &a) {
                          RequireValidType(*a.element_type, context);
                        }},
             type);
}

void ScopeVisitor::Visit(const StructDeclaration &node) {
  if (symbol_table_.HasStruct(node.name)) {
    throw std::runtime_error("Struct '" + node.name + "' is declared twice");
  }

  symbol_table_.AddStruct(StructInfo{node});

  std::string prev = current_struct_;
  current_struct_ = node.name;

  for (auto &m : node.methods) {
    m->Accept(*this);
  }

  current_struct_ = prev;
}

void ScopeVisitor::Visit(const MethodDeclaration &node) {
  if (inside_callable_) {
    throw std::runtime_error("Method '" + node.name +
                             "' cannot be declared inside a callable");
  }

  inside_callable_ = true;
  PushScope();

  if (const StructInfo *ci = symbol_table_.GetStruct(current_struct_)) {
    for (const auto &[fname, ftype] : ci->fields) {
      current_->DeclareLocal(fname,
                             VariableInfo(FieldDeclaration(fname, ftype)));
    }
  }

  for (auto &p : node.parameters) {
    if (!current_->DeclareLocal(p.name, VariableInfo(p))) {
      throw std::runtime_error("Duplicate parameter '" + p.name +
                               "' in method '" + node.name + "'");
    }
  }

  for (auto &s : node.body) {
    s->Accept(*this);
  }

  PopScope();
  inside_callable_ = false;
}

void ScopeVisitor::Visit(const IntLiteral &) {}
void ScopeVisitor::Visit(const BoolLiteral &) {}

void ScopeVisitor::Visit(const IdentityLiteral &node) {
  RequireDeclared(node.value);
}

void ScopeVisitor::Visit(const UnaryOperation &node) {
  node.operand->Accept(*this);
}

void ScopeVisitor::Visit(const BinaryOperation &node) {
  node.left_operand->Accept(*this);
  node.right_operand->Accept(*this);
}

void ScopeVisitor::Visit(const NewObjectExpression &node) {
  RequireValidType(StructType{node.struct_name}, "new object");
}

void ScopeVisitor::Visit(const NewArrayExpression &node) {
  RequireValidType(node.element_type, "new array");
  node.size->Accept(*this);
}

void ScopeVisitor::Visit(const ArrayIndexExpression &node) {

  if (const VariableInfo *info = RequireDeclared(node.name);
      !std::holds_alternative<ArrayType>(info->type)) {
    throw std::runtime_error("Variable '" + node.name +
                             "' is not an array type");
  }

  node.index->Accept(*this);
}

void ScopeVisitor::Visit(const ArrayAssignStatement &node) {

  if (const VariableInfo *info = RequireDeclared(node.name);
      !std::holds_alternative<ArrayType>(info->type)) {
    throw std::runtime_error("Variable '" + node.name +
                             "' is not an array type");
  }

  node.index->Accept(*this);
  node.value->Accept(*this);
}

void ScopeVisitor::Visit(const MethodCallExpression &node) {

  if (const VariableInfo *obj = RequireDeclared(node.object);
      std::holds_alternative<StructType>(obj->type)) {
    const std::string struct_name = std::get<StructType>(obj->type).name;

    if (const StructInfo *ci = symbol_table_.GetStruct(struct_name);
        ci && !ci->FindMethod(node.method_name)) {
      throw std::runtime_error("Method '" + node.method_name +
                               "' not found in struct '" + struct_name + "'");
    }
  }

  for (auto &a : node.args) {
    a->Accept(*this);
  }
}

void ScopeVisitor::Visit(const MethodCallStatement &node) {

  if (const VariableInfo *obj = RequireDeclared(node.object);
      std::holds_alternative<StructTYpe>(obj->type)) {
    const std::string struct_name = std::get<StructType>(obj->type).name;
    if (const StructInfo *ci = symbol_table_.GetStruct(struct_name);
        ci && !ci->FindMethod(node.method_name)) {
      throw std::runtime_error("Method '" + node.method_name +
                               "' not found in struct '" + struct_name + "'");
    }
  }

  for (auto &a : node.args) {
    a->Accept(*this);
  }
}

void ScopeVisitor::Visit(const FieldAccessExpression &node) {

  if (const VariableInfo *obj = RequireDeclared(node.object);
      std::holds_alternative<StructType>(obj->type)) {
    std::string struct_name = std::get<StructType>(obj->type).name;
    const StructInfo *ci = symbol_table_.GetStruct(struct_name);

    if (ci && !ci->FindField(node.field)) {
      throw std::runtime_error("Field '" + node.field +
                               "' not found in struct '" + struct_name + "'");
    }
  }
}

void ScopeVisitor::Visit(const VariableDeclaration &node) {
  node.value->Accept(*this);

  if (!current_->DeclareLocal(node.name, VariableInfo(node))) {
    throw std::runtime_error("Variable '" + node.name +
                             "' is already declared in this scope");
  }
}

void ScopeVisitor::Visit(const AssignStatement &node) {
  try {
    RequireDeclared(node.name);
  } catch (const std::exception &) {
    throw;
  }
  node.value->Accept(*this);
}

void ScopeVisitor::Visit(const IfStatement &node) {
  node.condition->Accept(*this);

  PushScope();
  for (auto &s : node.instructions) {
    s->Accept(*this);
  }
  PopScope();
}

void ScopeVisitor::Visit(const IfElseStatement &node) {
  node.condition->Accept(*this);

  PushScope();
  for (auto &s : node.then_instructions) {
    s->Accept(*this);
  }
  PopScope();

  PushScope();
  for (auto &s : node.else_instructions) {
    s->Accept(*this);
  }
  PopScope();
}

void ScopeVisitor::Visit(const WhileStatement &node) {
  node.condition->Accept(*this);

  PushScope();
  for (auto &s : node.body) {
    s->Accept(*this);
  }
  PopScope();
}

void ScopeVisitor::Visit(const PrintStatement &node) {
  node.expression->Accept(*this);
}

void ScopeVisitor::Visit(const FunctionDeclaration &node) {
  if (inside_callable_) {
    throw std::runtime_error("Function '" + node.name +
                             "' cannot be declared inside another function");
  }

  if (symbol_table_.HasFunction(node.name)) {
    throw std::runtime_error("Function '" + node.name + "' is declared twice");
  }

  symbol_table_.AddFunction(CallableInfo{node});
  inside_callable_ = true;

  PushScope();
  for (auto &p : node.parameters) {
    if (!current_->DeclareLocal(p.name, VariableInfo(p))) {
      throw std::runtime_error("Duplicate parameter '" + p.name +
                               "' in function '" + node.name + "'");
    }
  }

  for (auto &s : node.body) {
    s->Accept(*this);
  }
  PopScope();
  inside_callable_ = false;
}

void ScopeVisitor::Visit(const ReturnStatement &node) {
  node.value->Accept(*this);
}

void ScopeVisitor::Visit(const FunctionCallExpression &node) {
  if (!symbol_table_.HasFunction(node.function_name)) {
    throw std::runtime_error("Undefined function '" + node.function_name + "'");
  }

  for (auto &argument : node.args) {
    argument->Accept(*this);
  }
}

void ScopeVisitor::Visit(const FunctionCallStatement &node) {
  if (!symbol_table_.HasFunction(node.function_name)) {
    throw std::runtime_error("Undefined function '" + node.function_name + "'");
  }

  for (auto &argument : node.args) {
    argument->Accept(*this);
  }
}

void ScopeVisitor::Visit(const Program &node) {
  for (auto &c : node.structs) {
    c->Accept(*this);
  }

  for (auto &f : node.functions) {
    f->Accept(*this);
  }

  for (auto &s : node.instructions) {
    s->Accept(*this);
  }
}
