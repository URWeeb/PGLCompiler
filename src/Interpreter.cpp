#include "Interpreter.hpp"

Interpreter::ClassData::ClassData(const ClassDeclaration &node)
    : fields(&node.fields) {
  for (auto &m : node.methods) {
    methods[m->name] = m.get();
  }
}

int Interpreter::AsInt(const PossibleValue &v) { return std::get<int>(v); }

bool Interpreter::AsBool(const PossibleValue &v) { return std::get<bool>(v); }

ArrayData &Interpreter::AsArray(const PossibleValue &v) {
  return runtime_data_.arrays[std::get<ArrayID>(v).value];
}

ObjectData &Interpreter::AsObject(const PossibleValue &v) {
  return runtime_data_.objects[std::get<ObjectID>(v).value];
}

Interpreter::PossibleValue &Interpreter::GetLastValue() { return last_value_; }
Interpreter::PossibleValue Interpreter::GetLastValue() const {
  return last_value_;
}

std::unordered_map<std::string, Interpreter::PossibleValue> &
Interpreter::GetVariables() {
  return variables_;
}

std::unordered_map<std::string, Interpreter::PossibleValue>
Interpreter::GetVariables() const {
  return variables_;
}

Interpreter::PossibleValue &Interpreter::GetVar(const std::string &name) {
  return variables_[name];
}

Interpreter::PossibleValue Interpreter::GetVar(const std::string &name) const {
  return variables_.at(name);
}

void Interpreter::Visit(const IntLiteral &node) { last_value_ = node.value; }

void Interpreter::Visit(const BoolLiteral &node) { last_value_ = node.value; }

void Interpreter::Visit(const IdentityLiteral &node) {
  last_value_ = variables_.at(node.value);
}

void Interpreter::Visit(const UnaryOperation &node) {
  node.operand->Accept(*this);

  if (node.operation == "-") {
    last_value_ = -AsInt(last_value_);
  } else if (node.operation == "!") {
    last_value_ = !AsBool(last_value_);
  }
}

void Interpreter::Visit(const BinaryOperation &node) {
  node.left_operand->Accept(*this);
  const PossibleValue left = last_value_;
  node.right_operand->Accept(*this);
  const PossibleValue right = last_value_;

  if (node.operation == "+") {
    last_value_ = AsInt(left) + AsInt(right);
  } else if (node.operation == "-") {
    last_value_ = AsInt(left) - AsInt(right);
  } else if (node.operation == "*") {
    last_value_ = AsInt(left) * AsInt(right);
  } else if (node.operation == "/") {
    last_value_ = AsInt(left) / AsInt(right);
  } else if (node.operation == "==") {
    last_value_ = (left == right);
  } else if (node.operation == "!=") {
    last_value_ = (left != right);
  } else if (node.operation == "<=") {
    last_value_ = (AsInt(left) <= AsInt(right));
  } else if (node.operation == ">=") {
    last_value_ = (AsInt(left) >= AsInt(right));
  } else if (node.operation == "<") {
    last_value_ = (AsInt(left) < AsInt(right));
  } else if (node.operation == ">") {
    last_value_ = (AsInt(left) > AsInt(right));
  } else if (node.operation == "&&") {
    last_value_ = (AsBool(left) && AsBool(right));
  } else if (node.operation == "||") {
    last_value_ = (AsBool(left) || AsBool(right));
  }
}

void Interpreter::Visit(const VariableDeclaration &node) {
  node.value->Accept(*this);
  variables_[node.name] = last_value_;
}

void Interpreter::Visit(const AssignStatement &node) {
  node.value->Accept(*this);
  variables_[node.name] = last_value_;
}

void Interpreter::Visit(const ArrayAssignStatement &node) {
  node.index->Accept(*this);
  const int idx = AsInt(last_value_);
  node.value->Accept(*this);
  const PossibleValue val = last_value_;
  auto &[elements] = AsArray(variables_.at(node.name));

  if (idx < 0 || static_cast<size_t>(idx) >= elements.size()) {
    throw std::runtime_error("Array index " + std::to_string(idx) +
                             " out of bounds for '" + node.name + "'");
  }

  elements[static_cast<size_t>(idx)] = val;
}

void Interpreter::Visit(const IfStatement &node) {
  node.condition->Accept(*this);

  if (AsBool(last_value_)) {
    for (auto &s : node.instructions) {
      s->Accept(*this);
    }
  }
}

void Interpreter::Visit(const IfElseStatement &node) {
  node.condition->Accept(*this);

  if (AsBool(last_value_)) {
    for (auto &s : node.then_instructions) {
      s->Accept(*this);
    }
  } else {
    for (auto &s : node.else_instructions) {
      s->Accept(*this);
    }
  }
}

void Interpreter::Visit(const WhileStatement &node) {
  node.condition->Accept(*this);

  while (AsBool(last_value_)) {
    for (auto &s : node.body) {
      s->Accept(*this);
    }

    node.condition->Accept(*this);
  }
}

void Interpreter::Visit(const PrintStatement &node) {
  node.expression->Accept(*this);
  std::visit(
      [this]<typename T0>(const T0 &v) {
        using T = std::decay_t<T0>;

        if constexpr (std::is_same_v<T, int>) {
          std::cout << v << "\n";
        } else if constexpr (std::is_same_v<T, bool>) {
          std::cout << (v ? "true" : "false") << "\n";
        } else if constexpr (std::is_same_v<T, ArrayID>) {
          std::cout << "<array>\n";
        } else {
          std::cout << "<object:" << runtime_data_.objects[v.value].class_name
                    << ">\n";
        }
      },
      last_value_);
}

void Interpreter::Visit(const ReturnStatement &node) {
  node.value->Accept(*this);
  throw ReturnException{last_value_};
}

void Interpreter::Visit(const Program &node) {
  for (auto &c : node.classes) {
    c->Accept(*this);
  }

  for (auto &f : node.functions) {
    f->Accept(*this);
  }

  for (auto &s : node.instructions) {
    try {
      s->Accept(*this);
    } catch (ReturnException &e) {
      last_value_ = e.value;
      return;
    }
  }
}

void Interpreter::Visit(const NewArrayExpression &node) {
  node.size->Accept(*this);

  int sz = AsInt(last_value_);
  if (sz < 0) {
    throw std::runtime_error("Array size must be non-negative, got " +
                             std::to_string(sz));
  }

  PossibleValue zero;

  if (std::holds_alternative<IntType>(node.element_type)) {
    zero = 0;
  } else if (std::holds_alternative<BoolType>(node.element_type)) {
    zero = false;
  } else if (std::holds_alternative<ArrayType>(node.element_type)) {
    zero = ArrayID{0};
  } else {
    zero = ObjectID{0};
  }

  ArrayData data;
  data.elements.resize(static_cast<size_t>(sz), zero);
  last_value_ = runtime_data_.alloc_array(std::move(data));
}

void Interpreter::Visit(const ArrayIndexExpression &node) {
  node.index->Accept(*this);
  const int idx = AsInt(last_value_);
  const auto &[elements] = AsArray(variables_.at(node.name));

  if (idx < 0 || static_cast<size_t>(idx) >= elements.size()) {
    throw std::runtime_error("Array index " + std::to_string(idx) +
                             " out of bounds for '" + node.name + "'");
  }

  last_value_ = elements[static_cast<size_t>(idx)];
}

void Interpreter::Visit(const ClassDeclaration &node) {
  classes_.insert_or_assign(node.name, ClassData{node});
}

void Interpreter::Visit(const MethodDeclaration &) {}

void Interpreter::Visit(const NewObjectExpression &node) {
  ObjectData object;
  object.class_name = node.class_name;
  auto iter = classes_.find(node.class_name);

  if (iter != classes_.end()) {
    for (const auto &field : *iter->second.fields) {
      PossibleValue init_val;

      if (std::holds_alternative<IntType>(field.type)) {
        init_val = 0;
      } else if (std::holds_alternative<BoolType>(field.type)) {
        init_val = false;
      } else if (std::holds_alternative<ArrayType>(field.type)) {
        init_val = ArrayID{0};
      } else {
        init_val = ObjectID{0};
      }

      object.fields[field.name] = init_val;
    }
  }

  last_value_ = runtime_data_.alloc_object(std::move(object));
}

void Interpreter::Visit(const MethodCallExpression &node) {
  last_value_ = CallMethod(node.object, node.method_name, node.args);
}

void Interpreter::Visit(const MethodCallStatement &node) {
  CallMethod(node.object, node.method_name, node.args);
}

void Interpreter::Visit(const FieldAccessExpression &node) {
  last_value_ = AsObject(variables_.at(node.object)).fields.at(node.field);
}

void Interpreter::Visit(const FunctionDeclaration &node) {
  functions_[node.name] = &node;
}

void Interpreter::Visit(const FunctionCallExpression &node) {
  last_value_ = CallFunction(node.function_name, node.args);
}

void Interpreter::Visit(const FunctionCallStatement &node) {
  CallFunction(node.function_name, node.args);
}

Interpreter::PossibleValue Interpreter::CallFunction(
    const std::string &function_name,
    const std::vector<std::unique_ptr<Expression>> &arg_nodes) {
  auto iter = functions_.find(function_name);

  if (iter == functions_.end()) {
    throw std::runtime_error("Undefined function: '" + function_name + "'");
  }

  const FunctionDeclaration *function = iter->second;
  std::vector<PossibleValue> argument_values;
  argument_values.reserve(arg_nodes.size());

  for (auto &arg_node : arg_nodes) {
    arg_node->Accept(*this);
    argument_values.push_back(last_value_);
  }

  auto saved_variables = std::move(variables_);

  for (size_t i = 0; i < function->parameters.size(); ++i) {
    variables_[function->parameters[i].name] = argument_values[i];
  }

  PossibleValue return_value = {0};

  try {
    for (auto &s : function->body) {
      s->Accept(*this);
    }
  } catch (ReturnException &exception) {
    return_value = exception.value;
  }

  variables_ = std::move(saved_variables);
  return return_value;
}

Interpreter::PossibleValue Interpreter::CallMethod(
    const std::string &object_name, const std::string &method_name,
    const std::vector<std::unique_ptr<Expression>> &arg_nodes) {
  ObjectID object_id = std::get<ObjectID>(variables_.at(object_name));
  auto &class_data =
      classes_.at(runtime_data_.objects[object_id.value].class_name);
  auto method_iter = class_data.methods.find(method_name);

  if (method_iter == class_data.methods.end()) {
    throw std::runtime_error(
        "Method '" + method_name + "' not found in class '" +
        runtime_data_.objects[object_id.value].class_name + "'");
  }

  const MethodDeclaration *method = method_iter->second;
  std::vector<PossibleValue> argument_values;
  argument_values.reserve(arg_nodes.size());

  for (auto &arg_node : arg_nodes) {
    arg_node->Accept(*this);
    argument_values.push_back(last_value_);
  }

  auto saved_variables = std::move(variables_);
  variables_ = runtime_data_.objects[object_id.value].fields;

  for (size_t i = 0; i < method->parameters.size(); ++i) {
    variables_[method->parameters[i].name] = argument_values[i];
  }

  PossibleValue return_value = {0};

  try {
    for (auto &s : method->body) {
      s->Accept(*this);
    }
  } catch (ReturnException &exception) {
    return_value = exception.value;
  }

  for (const auto &field : *class_data.fields) {
    if (auto iter = variables_.find(field.name); iter != variables_.end()) {
      runtime_data_.objects[object_id.value].fields[field.name] = iter->second;
    }
  }

  variables_ = std::move(saved_variables);
  return return_value;
}
