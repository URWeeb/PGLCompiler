#pragma once

#include <memory>
#include <string>
#include <vector>

#include "types.hpp"

struct IntLiteral;
struct BoolLiteral;
struct IdentityLiteral;
struct UnaryOperation;
struct BinaryOperation;
struct VariableDeclaration;
struct AssignStatement;
struct ArrayAssignStatement;
struct IfStatement;
struct IfElseStatement;
struct WhileStatement;
struct PrintStatement;
struct StructDeclaration;
struct MethodDeclaration;
struct NewObjectExpression;
struct NewArrayExpression;
struct ArrayIndexExpression;
struct MethodCallExpression;
struct MethodCallStatement;
struct FieldAccessExpression;
struct FunctionDeclaration;
struct FunctionCallExpression;
struct FunctionCallStatement;
struct ReturnStatement;
struct Program;

struct Visitor {
    virtual void Visit(const IntLiteral &) = 0;

    virtual void Visit(const BoolLiteral &) = 0;

    virtual void Visit(const IdentityLiteral &) = 0;

    virtual void Visit(const UnaryOperation &) = 0;

    virtual void Visit(const BinaryOperation &) = 0;

    virtual void Visit(const VariableDeclaration &) = 0;

    virtual void Visit(const AssignStatement &) = 0;

    virtual void Visit(const ArrayAssignStatement &) = 0;

    virtual void Visit(const IfStatement &) = 0;

    virtual void Visit(const IfElseStatement &) = 0;

    virtual void Visit(const WhileStatement &) = 0;

    virtual void Visit(const PrintStatement &) = 0;

    virtual void Visit(const StructDeclaration &) = 0;

    virtual void Visit(const MethodDeclaration &) = 0;

    virtual void Visit(const NewObjectExpression &) = 0;

    virtual void Visit(const NewArrayExpression &) = 0;

    virtual void Visit(const ArrayIndexExpression &) = 0;

    virtual void Visit(const MethodCallExpression &) = 0;

    virtual void Visit(const MethodCallStatement &) = 0;

    virtual void Visit(const FieldAccessExpression &) = 0;

    virtual void Visit(const FunctionDeclaration &) = 0;

    virtual void Visit(const FunctionCallExpression &) = 0;

    virtual void Visit(const FunctionCallStatement &) = 0;

    virtual void Visit(const ReturnStatement &) = 0;

    virtual void Visit(const Program &) = 0;

    virtual ~Visitor() = default;
};

struct Node {
    virtual void Accept(Visitor &) = 0;

    virtual ~Node() = default;
};

struct Statement : Node {
};

using Statements = std::vector<std::unique_ptr<Statement> >;

struct Expression : Node {
};

struct IntLiteral : Expression {
    int value;

    IntLiteral(int val) : value(val) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct BoolLiteral : Expression {
    bool value;

    BoolLiteral(bool val) : value(val) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct IdentityLiteral : Expression {
    std::string value;

    IdentityLiteral(std::string val) : value(std::move(val)) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct StorageDeclaration {
    std::string name;
    Type type;

    explicit StorageDeclaration() = default;

    StorageDeclaration(std::string nm, Type tp)
        : name(std::move(nm)), type(std::move(tp)) {
    }
};

struct Parameter : StorageDeclaration {
    using StorageDeclaration::StorageDeclaration;
};

struct FieldDeclaration : StorageDeclaration {
    using StorageDeclaration::StorageDeclaration;
};

struct UnaryOperation : Expression {
    std::string operation;
    std::unique_ptr<Expression> operand;

    UnaryOperation(Expression *opnd, std::string option)
        : operation(std::move(option)), operand(opnd) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct BinaryOperation : Expression {
    std::string operation;
    std::unique_ptr<Expression> left_operand;
    std::unique_ptr<Expression> right_operand;

    BinaryOperation(Expression *l_operand, Expression *r_operand, std::string op)
        : operation(std::move(op)),
          left_operand(l_operand),
          right_operand(r_operand) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct NewObjectExpression : Expression {
    std::string struct_name;

    NewObjectExpression(std::string cl_name) : struct_name(std::move(cl_name)) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct NewArrayExpression : Expression {
    Type element_type;
    std::unique_ptr<Expression> size;

    NewArrayExpression(Type el_type, Expression *sz)
        : element_type(std::move(el_type)), size(sz) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct ArrayIndexExpression : Expression {
    std::string name;
    std::unique_ptr<Expression> index;

    ArrayIndexExpression(std::string nm, Expression *idx)
        : name(std::move(nm)), index(idx) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct MethodCallExpression : Expression {
    std::string object;
    std::string method_name;
    std::vector<std::unique_ptr<Expression> > args;

    MethodCallExpression(std::string obj, std::string meth_name,
                         std::vector<std::unique_ptr<Expression> > &&arguments)
        : object(std::move(obj)),
          method_name(std::move(meth_name)),
          args(std::move(arguments)) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct FieldAccessExpression : Expression {
    std::string object;
    std::string field;

    FieldAccessExpression(std::string obj, std::string fld)
        : object(std::move(obj)), field(std::move(fld)) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct VariableDeclaration : StorageDeclaration, Statement {
    std::unique_ptr<Expression> value;

    VariableDeclaration(std::string nm, Type tp, Expression *val)
        : StorageDeclaration(std::move(nm), std::move(tp)), value(val) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct AssignStatement : Statement {
    std::string name;
    std::unique_ptr<Expression> value;

    AssignStatement(std::string nm, Expression *val)
        : name(std::move(nm)), value(val) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct ArrayAssignStatement : Statement {
    std::string name;
    std::unique_ptr<Expression> index;
    std::unique_ptr<Expression> value;

    ArrayAssignStatement(std::string nm, Expression *idx, Expression *val)
        : name(std::move(nm)), index(idx), value(val) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct IfStatement : Statement {
    std::unique_ptr<Expression> condition;
    Statements instructions;

    IfStatement(Expression *cond, Statements &&insts)
        : condition(cond), instructions(std::move(insts)) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct IfElseStatement : Statement {
    std::unique_ptr<Expression> condition;
    Statements then_instructions;
    Statements else_instructions;

    IfElseStatement(Expression *cond, Statements &&then_insts,
                    Statements &&else_insts)
        : condition(cond),
          then_instructions(std::move(then_insts)),
          else_instructions(std::move(else_insts)) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct WhileStatement : Statement {
    std::unique_ptr<Expression> condition;
    Statements body;

    WhileStatement(Expression *cond, Statements &&bdy)
        : condition(cond), body(std::move(bdy)) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct PrintStatement : Statement {
    std::unique_ptr<Expression> expression;

    PrintStatement(Expression *expr) : expression(expr) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct ReturnStatement : Statement {
    std::unique_ptr<Expression> value;

    ReturnStatement(Expression *val) : value(val) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct MethodDeclaration : Node {
    std::string name;
    std::vector<Parameter> parameters;
    Type return_type;
    Statements body;

    MethodDeclaration(std::string nm, std::vector<Parameter> &&params,
                      Type ret_type, Statements &&bdy)
        : name(std::move(nm)),
          parameters(std::move(params)),
          return_type(std::move(ret_type)),
          body(std::move(bdy)) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct StructDeclaration : Node {
    std::string name;
    std::vector<FieldDeclaration> fields;
    std::vector<std::unique_ptr<MethodDeclaration> > methods;

    StructDeclaration(std::string nm, std::vector<FieldDeclaration> &&flds,
                     std::vector<std::unique_ptr<MethodDeclaration> > &&meths)
        : name(std::move(nm)),
          fields(std::move(flds)),
          methods(std::move(meths)) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct MethodCallStatement : Statement {
    std::string object;
    std::string method_name;
    std::vector<std::unique_ptr<Expression> > args;

    MethodCallStatement(std::string obj, std::string meth_name,
                        std::vector<std::unique_ptr<Expression> > &&arguments)
        : object(std::move(obj)),
          method_name(std::move(meth_name)),
          args(std::move(arguments)) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct FunctionDeclaration : Node {
    std::string name;
    std::vector<Parameter> parameters;
    Type return_type;
    Statements body;

    FunctionDeclaration(std::string nm, std::vector<Parameter> &&params,
                        Type ret_type, Statements &&bdy)
        : name(std::move(nm)),
          parameters(std::move(params)),
          return_type(std::move(ret_type)),
          body(std::move(bdy)) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct FunctionCallExpression : Expression {
    std::string function_name;
    std::vector<std::unique_ptr<Expression> > args;

    FunctionCallExpression(std::string func_name,
                           std::vector<std::unique_ptr<Expression> > &&arguments)
        : function_name(std::move(func_name)), args(std::move(arguments)) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct FunctionCallStatement : Statement {
    std::string function_name;
    std::vector<std::unique_ptr<Expression> > args;

    FunctionCallStatement(std::string func_name,
                          std::vector<std::unique_ptr<Expression> > &&arguments)
        : function_name(std::move(func_name)), args(std::move(arguments)) {
    }

    void Accept(Visitor &v) override { v.Visit(*this); }
};

struct Program {
    std::vector<std::unique_ptr<StructDeclaration> > structs;
    std::vector<std::unique_ptr<FunctionDeclaration> > functions;
    Statements instructions;

    Program(std::vector<std::unique_ptr<StructDeclaration> > &&strcs,
            std::vector<std::unique_ptr<FunctionDeclaration> > &&funcs,
            Statements &&insts)
        : structs(std::move(strcs)),
          functions(std::move(funcs)),
          instructions(std::move(insts)) {
    }

    Program(std::vector<std::unique_ptr<StructDeclaration> > &&strcs,
            Statements &&insts)
        : structs(std::move(strcs)), instructions(std::move(insts)) {
    }

    void Accept(Visitor &v) { v.Visit(*this); }
};
