#include "TypeChecker.hpp"

#include <ranges>

TypeChecker::TypeChecker(const SymbolTable &symbol_table)
    : symbol_table_(symbol_table) {
  PushScope();
}

void TypeChecker::PushScope() { scopes_.emplace_back(); }
void TypeChecker::PopScope() { scopes_.pop_back(); }

void TypeChecker::DeclareVariable(const std::string &name, const Type &type) {
  scopes_.back()[name] = type;
}

Type TypeChecker::GetVariableType(const std::string &name) const {
  for (const auto &scope : std::views::reverse(scopes_)) {
    if (scope.contains(name)) {
      return scope.at(name);
    }
  }

  throw std::runtime_error("Undeclared variable '" + name + "'");
}

void TypeChecker::ExpectType(const Type &actual, const Type &expected,
                             const std::string &context) {
  if (actual != expected) {
    throw std::runtime_error(context + ": expected type '" +
                             TypeToString(expected) + "', but got '" +
                             TypeToString(actual) + "'");
  }
}

void TypeChecker::CheckMethodCall(
    const std::string &object_name, const std::string &method_name,
    const std::vector<std::unique_ptr<Expression>> &args) {
  Type object_type = GetVariableType(object_name);

  if (!std::holds_alternative<StructType>(object_type)) {
    throw std::runtime_error("Cannot call method on non-object '" +
                             object_name + "'");
  }

  std::string struct_name = std::get<StructType>(object_type).name;
  const StructInfo *struct_info = symbol_table_.GetStruct(struct_name);

  if (struct_info == nullptr) {
    throw std::runtime_error("Unknown struct '" + struct_name + "'");
  }

  const CallableInfo *method = struct_info->FindMethod(method_name);

  if (method == nullptr) {
    throw std::runtime_error("Method '" + method_name +
                             "' not found in struct '" + struct_name + "'");
  }

  if (method->params.size() != args.size()) {
    throw std::runtime_error("Argument count mismatch in method call '" +
                             method_name + "'");
  }

  for (size_t i = 0; i < args.size(); ++i) {
    args[i]->Accept(*this);
    ExpectType(current_type_, method->params[i].type,
               "Argument " + std::to_string(i) + " of method '" + method_name +
                   "'");
  }

  current_type_ = method->return_type;
}

void TypeChecker::CheckFunctionCall(
    const std::string &function_name,
    const std::vector<std::unique_ptr<Expression>> &args) {
  const CallableInfo *function = symbol_table_.GetFunction(function_name);

  if (function == nullptr) {
    throw std::runtime_error("Call to undefined function '" + function_name +
                             "'");
  }

  if (function->params.size() != args.size()) {
    throw std::runtime_error("Argument count mismatch in function call '" +
                             function_name + "'");
  }

  for (size_t i = 0; i < args.size(); ++i) {
    args[i]->Accept(*this);
    ExpectType(current_type_, function->params[i].type,
               "Argument " + std::to_string(i) + " of function '" +
                   function_name + "'");
  }

  current_type_ = function->return_type;
}

void TypeChecker::Visit(const IntLiteral &) { current_type_ = IntType{}; }

void TypeChecker::Visit(const BoolLiteral &) { current_type_ = BoolType{}; }

void TypeChecker::Visit(const IdentityLiteral &node) {
  current_type_ = GetVariableType(node.value);
}

void TypeChecker::Visit(const UnaryOperation &node) {
  node.operand->Accept(*this);

  if (node.operation == "!") {
    ExpectType(current_type_, BoolType{}, "Unary '!' operand");
    current_type_ = BoolType{};
  } else if (node.operation == "-") {
    ExpectType(current_type_, IntType{}, "Unary '-' operand");
    current_type_ = IntType{};
  }
}

void TypeChecker::Visit(const BinaryOperation &node) {
  node.left_operand->Accept(*this);
  Type left_type = current_type_;
  node.right_operand->Accept(*this);
  Type right_type = current_type_;

  if (node.operation == "+" || node.operation == "-" || node.operation == "*" ||
      node.operation == "/") {
    ExpectType(left_type, IntType{}, "Left operand of arithmetic operation");
    ExpectType(right_type, IntType{}, "Right operand of arithmetic operation");
    current_type_ = IntType{};
  } else if (node.operation == "==" || node.operation == "!=") {
    if (left_type != right_type) {
      throw std::runtime_error("Type mismatch in comparison: '" +
                               TypeToString(left_type) + "' vs '" +
                               TypeToString(right_type) + "'");
    }

    current_type_ = BoolType{};
  } else if (node.operation == "<" || node.operation == ">" ||
             node.operation == "<=" || node.operation == ">=") {
    ExpectType(left_type, IntType{}, "Left operand of relation");
    ExpectType(right_type, IntType{}, "Right operand of relation");
    current_type_ = BoolType{};
  } else if (node.operation == "&&" || node.operation == "||") {
    ExpectType(left_type, BoolType{}, "Left operand of logical operation");
    ExpectType(right_type, BoolType{}, "Right operand of logical operation");
    current_type_ = BoolType{};
  }
}

void TypeChecker::Visit(const VariableDeclaration &node) {
  node.value->Accept(*this);
  ExpectType(current_type_, node.type,
             "Variable initialization for '" + node.name + "'");
  DeclareVariable(node.name, node.type);
}

void TypeChecker::Visit(const AssignStatement &node) {
  node.value->Accept(*this);
  ExpectType(current_type_, GetVariableType(node.name),
             "Assignment to '" + node.name + "'");
}

void TypeChecker::Visit(const ArrayAssignStatement &node) {
  Type arr_type = GetVariableType(node.name);

  if (!std::holds_alternative<ArrayType>(arr_type)) {
    throw std::runtime_error("Variable '" + node.name + "' is not an array");
  }

  Type base_type = *std::get<ArrayType>(arr_type).element_type;
  node.index->Accept(*this);
  ExpectType(current_type_, IntType{}, "Array index");
  node.value->Accept(*this);
  ExpectType(current_type_, base_type, "Array assignment value");
}

void TypeChecker::Visit(const IfStatement &node) {
  node.condition->Accept(*this);
  ExpectType(current_type_, BoolType{}, "If condition");

  PushScope();
  for (const auto &instruction : node.instructions) {
    instruction->Accept(*this);
  }
  PopScope();
}

void TypeChecker::Visit(const IfElseStatement &node) {
  node.condition->Accept(*this);
  ExpectType(current_type_, BoolType{}, "If condition");

  PushScope();
  for (const auto &instruction : node.then_instructions) {
    instruction->Accept(*this);
  }
  PopScope();

  PushScope();
  for (const auto &instruction : node.else_instructions) {
    instruction->Accept(*this);
  }
  PopScope();
}

void TypeChecker::Visit(const WhileStatement &node) {
  node.condition->Accept(*this);
  ExpectType(current_type_, BoolType{}, "While condition");

  PushScope();
  for (const auto &instruction : node.body) {
    instruction->Accept(*this);
  }
  PopScope();
}

void TypeChecker::Visit(const PrintStatement &node) {
  node.expression->Accept(*this);
}

void TypeChecker::Visit(const StructDeclaration &node) {
  std::string prev_struct = current_struct_;
  current_struct_ = node.name;

  for (const auto &method : node.methods) {
    method->Accept(*this);
  }

  current_struct_ = prev_struct;
}

void TypeChecker::Visit(const MethodDeclaration &node) {
  PushScope();

  if (const StructInfo *struct_info = symbol_table_.GetStruct(current_struct_)) {
    for (const auto &[fname, ftype] : struct_info->fields) {
      DeclareVariable(fname, ftype);
    }
  }

  for (const auto &parameter : node.parameters) {
    DeclareVariable(parameter.name, parameter.type);
  }

  Type prev_return = expected_return_type_;
  expected_return_type_ = node.return_type;

  for (const auto &instruction : node.body) {
    instruction->Accept(*this);
  }

  expected_return_type_ = prev_return;
  PopScope();
}

void TypeChecker::Visit(const NewObjectExpression &node) {
  if (!symbol_table_.HasStruct(node.struct_name)) {
    throw std::runtime_error("Cannot instantiate unknown struct: '" +
                             node.struct_name + "'");
  }

  current_type_ = StructType{node.struct_name};
}

void TypeChecker::Visit(const NewArrayExpression &node) {
  node.size->Accept(*this);
  ExpectType(current_type_, IntType{}, "Array size");
  current_type_ = ArrayType{std::make_unique<Type>(node.element_type)};
}

void TypeChecker::Visit(const ArrayIndexExpression &node) {
  Type arr_type = GetVariableType(node.name);

  if (!std::holds_alternative<ArrayType>(arr_type)) {
    throw std::runtime_error("Cannot index non-array variable '" + node.name +
                             "'");
  }

  node.index->Accept(*this);
  ExpectType(current_type_, IntType{}, "Array index");

  current_type_ = *std::get<ArrayType>(arr_type).element_type;
}

void TypeChecker::Visit(const MethodCallExpression &node) {
  CheckMethodCall(node.object, node.method_name, node.args);
}

void TypeChecker::Visit(const MethodCallStatement &node) {
  CheckMethodCall(node.object, node.method_name, node.args);
}

void TypeChecker::Visit(const FieldAccessExpression &node) {
  Type obj_type = GetVariableType(node.object);

  if (!std::holds_alternative<Structtype>(obj_type)) {
    throw std::runtime_error("Variable '" + node.object + "' is not an object");
  }

  std::string struct_name = std::get<StructType>(obj_type).name;
  const StructInfo *struct_info = symbol_table_.GetStruct(struct_name);

  if (struct_info == nullptr) {
    throw std::runtime_error("Unknown struct '" + struct_name + "'");
  }

  const Type *field_type = struct_info->FindField(node.field);

  if (field_type == nullptr) {
    throw std::runtime_error("Field '" + node.field + "' not found in struct '" +
                             struct_name + "'");
  }

  current_type_ = *field_type;
}

void TypeChecker::Visit(const FunctionDeclaration &node) {
  PushScope();

  for (const auto &parameter : node.parameters) {
    DeclareVariable(parameter.name, parameter.type);
  }

  Type prev_return = expected_return_type_;
  expected_return_type_ = node.return_type;

  for (const auto &instruction : node.body) {
    instruction->Accept(*this);
  }

  expected_return_type_ = prev_return;
  PopScope();
}

void TypeChecker::Visit(const FunctionCallExpression &node) {
  CheckFunctionCall(node.function_name, node.args);
}

void TypeChecker::Visit(const FunctionCallStatement &node) {
  CheckFunctionCall(node.function_name, node.args);
}

void TypeChecker::Visit(const ReturnStatement &node) {
  node.value->Accept(*this);
  ExpectType(current_type_, expected_return_type_, "Return statement");
}

void TypeChecker::Visit(const Program &node) {
  for (const auto &clas : node.structs) {
    clas->Accept(*this);
  }

  for (const auto &function : node.functions) {
    function->Accept(*this);
  }

  Type prev_return = expected_return_type_;
  expected_return_type_ = IntType{};

  for (const auto &instruction : node.instructions) {
    instruction->Accept(*this);
  }

  expected_return_type_ = prev_return;
}
