#include "PrintVisitor.hpp"

PrintVisitor::PrintVisitor(const std::string &filepath) : output_(filepath) {}

void PrintVisitor::Write(const std::string &str) {
  output_ << std::string(indent_ * 4, ' ') << str << "\n";
}

void PrintVisitor::Visit(const IntLiteral &node) {
  Write("IntLiteral: " + std::to_string(node.value));
}

void PrintVisitor::Visit(const BoolLiteral &node) {
  Write(std::string("BoolLiteral: ") + (node.value ? "true" : "false"));
}

void PrintVisitor::Visit(const IdentityLiteral &node) {
  Write("IdentityLiteral: " + node.value);
}

void PrintVisitor::Visit(const UnaryOperation &node) {
  Write("UnaryOperation: " + node.operation);

  ++indent_;
  node.operand->Accept(*this);
  --indent_;
}

void PrintVisitor::Visit(const BinaryOperation &node) {
  Write("BinaryOperation: " + node.operation);

  ++indent_;
  node.left_operand->Accept(*this);
  node.right_operand->Accept(*this);
  --indent_;
}

void PrintVisitor::Visit(const VariableDeclaration &node) {
  Write("Var " + node.name + " : " + TypeToString(node.type));

  ++indent_;
  node.value->Accept(*this);
  --indent_;
}

void PrintVisitor::Visit(const AssignStatement &node) {
  Write("Assign " + node.name + " =");

  ++indent_;
  node.value->Accept(*this);
  --indent_;
}

void PrintVisitor::Visit(const ArrayAssignStatement &node) {
  Write("ArrayAssign " + node.name + "[i] =");

  ++indent_;
  Write("Index:");

  ++indent_;
  node.index->Accept(*this);
  --indent_;

  Write("Value:");

  ++indent_;
  node.value->Accept(*this);
  indent_ -= 2;
}

void PrintVisitor::Visit(const IfStatement &node) {
  Write("If");

  ++indent_;
  Write("Condition:");

  ++indent_;
  node.condition->Accept(*this);
  --indent_;

  Write("Then:");

  ++indent_;
  for (auto &s : node.instructions) {
    s->Accept(*this);
  }
  indent_ -= 2;
}

void PrintVisitor::Visit(const IfElseStatement &node) {
  Write("If");

  ++indent_;
  Write("Condition:");

  ++indent_;
  node.condition->Accept(*this);
  --indent_;

  Write("Then:");

  ++indent_;
  for (auto &s : node.then_instructions) {
    s->Accept(*this);
  }
  --indent_;

  Write("Else:");

  ++indent_;
  for (auto &s : node.else_instructions) {
    s->Accept(*this);
  }
  indent_ -= 2;
}

void PrintVisitor::Visit(const WhileStatement &node) {
  Write("While");

  ++indent_;
  Write("Condition:");

  ++indent_;
  node.condition->Accept(*this);
  --indent_;

  Write("Body:");

  ++indent_;
  for (auto &s : node.body) {
    s->Accept(*this);
  }
  indent_ -= 2;
}

void PrintVisitor::Visit(const PrintStatement &node) {
  Write("Print:");

  ++indent_;
  node.expression->Accept(*this);
  --indent_;
}

void PrintVisitor::Visit(const ReturnStatement &node) {
  Write("Return:");

  ++indent_;
  node.value->Accept(*this);
  --indent_;
}

void PrintVisitor::Visit(const StructDeclaration &node) {
  Write("Struct: " + node.name);

  ++indent_;
  for (auto &f : node.fields) {
    Write("Field " + f.name + " : " + TypeToString(f.type));
  }
  for (auto &m : node.methods) {
    m->Accept(*this);
  }
  --indent_;
}

void PrintVisitor::Visit(const MethodDeclaration &node) {
  std::string sig = "Method " + node.name + "(";

  for (size_t i = 0; i < node.parameters.size(); ++i) {
    if (i > 0) {
      sig += ", ";
    }
    sig +=
        node.parameters[i].name + ": " + TypeToString(node.parameters[i].type);
  }

  sig += ") -> " + TypeToString(node.return_type);
  Write(sig);

  ++indent_;
  for (auto &s : node.body) {
    s->Accept(*this);
  }
  --indent_;
}

void PrintVisitor::Visit(const FunctionDeclaration &node) {
  std::string sig = "Function " + node.name + "(";

  for (size_t i = 0; i < node.parameters.size(); ++i) {
    if (i > 0) {
      sig += ", ";
    }
    sig +=
        node.parameters[i].name + ": " + TypeToString(node.parameters[i].type);
  }

  sig += ") -> " + TypeToString(node.return_type);
  Write(sig);

  ++indent_;
  for (auto &s : node.body) {
    s->Accept(*this);
  }
  --indent_;
}

void PrintVisitor::Visit(const NewObjectExpression &node) {
  Write("NewObject: " + node.struct_name);
}

void PrintVisitor::Visit(const NewArrayExpression &node) {
  Write("NewArray: " + TypeToString(node.element_type) + "[]");

  ++indent_;
  node.size->Accept(*this);
  --indent_;
}

void PrintVisitor::Visit(const ArrayIndexExpression &node) {
  Write("ArrayIndex: " + node.name + "[i]");

  ++indent_;
  node.index->Accept(*this);
  --indent_;
}

void PrintVisitor::Visit(const MethodCallExpression &node) {
  Write("MethodCall: " + node.object + "." + node.method_name);

  ++indent_;
  for (auto &a : node.args) {
    a->Accept(*this);
  }
  --indent_;
}

void PrintVisitor::Visit(const MethodCallStatement &node) {
  Write("MethodCallStatement: " + node.object + "." + node.method_name);

  ++indent_;
  for (auto &a : node.args) {
    a->Accept(*this);
  }
  --indent_;
}

void PrintVisitor::Visit(const FunctionCallExpression &node) {
  Write("FunctionCallExpression: " + node.function_name);

  ++indent_;
  for (auto &a : node.args) {
    a->Accept(*this);
  }
  --indent_;
}

void PrintVisitor::Visit(const FunctionCallStatement &node) {
  Write("FunctionCallStatement: " + node.function_name);

  ++indent_;
  for (auto &a : node.args) {
    a->Accept(*this);
  }
  --indent_;
}

void PrintVisitor::Visit(const FieldAccessExpression &node) {
  Write("FieldAccess: " + node.object + "." + node.field);
}

void PrintVisitor::Visit(const Program &node) {
  Write("Program:");

  ++indent_;
  for (auto &c : node.structs) {
    c->Accept(*this);
  }

  for (auto &f : node.functions) {
    f->Accept(*this);
  }

  for (auto &s : node.instructions) {
    s->Accept(*this);
  }
  --indent_;
}
