#include <gtest/gtest.h>

#include <fstream>
#include <memory>
#include <optional>
#include <unistd.h>
#include <sstream>
#include <string>
#include <vector>

#include "tree.hpp"
#include "types.hpp"
#include "visitors.hpp"

struct RunResult {
  int exit_code;
  std::string err_output;
};

static RunResult run_compiler(const std::string &input) {
  char tempfile[] = "/tmp/compiler_test_XXXXXX";
  int fd = mkstemp(tempfile);
  write(fd, input.c_str(), input.size());
  close(fd);

  std::string cmd = std::string("./compiler < ") + tempfile +
                    " 2>/tmp/compiler_stderr_XXXXXX";
  const std::string full_cmd =
      std::string("./compiler < ") + tempfile + " 2>&1 >/dev/null";

  if (FILE *pipe = popen(full_cmd.c_str(), "r")) {
    std::string err;
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe)) {
      err += buf;
    }

    int status = pclose(pipe);
    remove(tempfile);
    return {WEXITSTATUS(status), err};
  }

  remove(tempfile);
  return {-1, ""};
}

TEST(IntLiteralTest, StoresValue) {
  IntLiteral node(42);
  EXPECT_EQ(node.value, 42);
}

TEST(IntLiteralTest, NegativeValue) {
  IntLiteral node(-7);
  EXPECT_EQ(node.value, -7);
}

TEST(IntLiteralTest, Zero) {
  IntLiteral node(0);
  EXPECT_EQ(node.value, 0);
}

TEST(IntLiteralTest, IsExpression) {
  IntLiteral node(1);
  Expression *expr = &node;
  EXPECT_NE(expr, nullptr);
}

TEST(BoolLiteralTest, TrueValue) {
  BoolLiteral node(true);
  EXPECT_TRUE(node.value);
}

TEST(BoolLiteralTest, FalseValue) {
  BoolLiteral node(false);
  EXPECT_FALSE(node.value);
}

TEST(BoolLiteralTest, IsExpression) {
  BoolLiteral node(true);
  Expression *expr = &node;
  EXPECT_NE(expr, nullptr);
}

TEST(IdentityLiteralTest, StoresName) {
  IdentityLiteral node("myVar");
  EXPECT_EQ(node.value, "myVar");
}

TEST(IdentityLiteralTest, IsExpression) {
  IdentityLiteral node("x");
  Expression *expr = &node;
  EXPECT_NE(expr, nullptr);
}

TEST(UnaryOperationTest, NotOperation) {
  auto *operand = new BoolLiteral(true);
  UnaryOperation node(operand, "!");
  EXPECT_EQ(node.operation, "!");
  EXPECT_NE(node.operand, nullptr);
}

TEST(UnaryOperationTest, NegationOperation) {
  auto *operand = new IntLiteral(5);
  UnaryOperation node(operand, "-");
  EXPECT_EQ(node.operation, "-");
}

TEST(UnaryOperationTest, OwnsOperand) {
  auto *raw = new IntLiteral(10);
  UnaryOperation node(raw, "!");
  EXPECT_EQ(node.operand.get(), raw);
}

TEST(UnaryOperationTest, IsExpression) {
  auto *operand = new IntLiteral(1);
  UnaryOperation node(operand, "!");
  Expression *expr = &node;
  EXPECT_NE(expr, nullptr);
}

TEST(BinaryOperationTest, PlusOperation) {
  auto *left = new IntLiteral(2);
  auto *right = new IntLiteral(3);
  BinaryOperation node(left, right, "+");
  EXPECT_EQ(node.operation, "+");
  EXPECT_NE(node.left_operand, nullptr);
  EXPECT_NE(node.right_operand, nullptr);
}

TEST(BinaryOperationTest, AllArithmeticOps) {
  for (const std::string &op : {"-", "*", "/"}) {
    auto *left = new IntLiteral(1);
    auto *right = new IntLiteral(2);
    BinaryOperation node(left, right, op);
    EXPECT_EQ(node.operation, op);
  }
}

TEST(BinaryOperationTest, ComparisonOps) {
  for (const std::string &op : {"==", "!=", "<", ">", "<=", ">="}) {
    auto *left = new IntLiteral(0);
    auto *right = new IntLiteral(1);
    BinaryOperation node(left, right, op);
    EXPECT_EQ(node.operation, op);
  }
}

TEST(BinaryOperationTest, LogicalOps) {
  for (const std::string &op : {"&&", "||"}) {
    auto *left = new BoolLiteral(true);
    auto *right = new BoolLiteral(false);
    BinaryOperation node(left, right, op);
    EXPECT_EQ(node.operation, op);
  }
}

TEST(BinaryOperationTest, OwnsOperands) {
  auto *l = new IntLiteral(1);
  auto *r = new IntLiteral(2);
  BinaryOperation node(l, r, "+");
  EXPECT_EQ(node.left_operand.get(), l);
  EXPECT_EQ(node.right_operand.get(), r);
}

TEST(BinaryOperationTest, IsExpression) {
  auto *l = new IntLiteral(1);
  auto *r = new IntLiteral(2);
  BinaryOperation node(l, r, "+");
  Expression *expr = &node;
  EXPECT_NE(expr, nullptr);
}

TEST(VariableDeclarationTest, IntDeclaration) {
  auto *val = new IntLiteral(0);
  VariableDeclaration decl("x", IntType{}, val);
  EXPECT_EQ(decl.name, "x");
  EXPECT_EQ(decl.type, Type{IntType{}});
  EXPECT_NE(decl.value, nullptr);
}

TEST(VariableDeclarationTest, BoolDeclaration) {
  auto *val = new BoolLiteral(false);
  VariableDeclaration decl("flag", BoolType{}, val);
  EXPECT_EQ(decl.name, "flag");
  EXPECT_EQ(decl.type, Type{BoolType{}});
}

TEST(VariableDeclarationTest, DeletesNameAndType) {
  auto *val = new IntLiteral(999);
  VariableDeclaration decl("myVar", IntType{}, val);
  EXPECT_EQ(decl.name, "myVar");
  EXPECT_EQ(decl.type, Type{IntType{}});
}

TEST(VariableDeclarationTest, IsStatement) {
  auto *val = new IntLiteral(1);
  VariableDeclaration decl("y", IntType{}, val);
  Statement *stmt = &decl;
  EXPECT_NE(stmt, nullptr);
}

TEST(AssignStatementTest, StoresNameAndValue) {
  auto *val = new IntLiteral(10);
  AssignStatement stmt("counter", val);
  EXPECT_EQ(stmt.name, "counter");
  EXPECT_NE(stmt.value, nullptr);
}

TEST(AssignStatementTest, DeletesName) {
  auto *val = new BoolLiteral(true);
  AssignStatement stmt("z", val);
  EXPECT_EQ(stmt.name, "z");
}

TEST(AssignStatementTest, IsStatement) {
  auto *val = new IntLiteral(0);
  AssignStatement stmt("a", val);
  Statement *s = &stmt;
  EXPECT_NE(s, nullptr);
}

TEST(IfStatementTest, EmptyBody) {
  auto *cond = new BoolLiteral(true);
  Statements stmts{};
  IfStatement node(cond, std::move(stmts));
  EXPECT_NE(node.condition, nullptr);
  EXPECT_TRUE(node.instructions.empty());
}

TEST(IfStatementTest, WithBody) {
  auto *cond = new BoolLiteral(false);
  Statements stmts{};
  stmts.push_back(
      std::make_unique<VariableDeclaration>("x", IntType{}, new IntLiteral(1)));
  IfStatement node(cond, std::move(stmts));
  EXPECT_EQ(node.instructions.size(), 1u);
}

TEST(IfStatementTest, IsStatement) {
  auto *cond = new BoolLiteral(true);
  Statements stmts{};
  IfStatement node(cond, std::move(stmts));
  Statement *s = &node;
  EXPECT_NE(s, nullptr);
}

TEST(IfElseStatementTest, EmptyBranches) {
  auto *cond = new BoolLiteral(true);
  Statements then_stmts{};
  Statements else_stmts{};
  IfElseStatement node(cond, std::move(then_stmts), std::move(else_stmts));
  EXPECT_NE(node.condition, nullptr);
  EXPECT_TRUE(node.then_instructions.empty());
  EXPECT_TRUE(node.else_instructions.empty());
}

TEST(IfElseStatementTest, BothBranchesHaveStatements) {
  auto *cond = new BoolLiteral(true);
  Statements then_stmts{};
  Statements else_stmts{};

  then_stmts.push_back(
      std::make_unique<VariableDeclaration>("a", IntType{}, new IntLiteral(1)));
  else_stmts.push_back(std::make_unique<VariableDeclaration>(
      "b", BoolType{}, new BoolLiteral(false)));

  IfElseStatement node(cond, std::move(then_stmts), std::move(else_stmts));
  EXPECT_EQ(node.then_instructions.size(), 1u);
  EXPECT_EQ(node.else_instructions.size(), 1u);
}

TEST(IfElseStatementTest, IsStatement) {
  auto *cond = new BoolLiteral(true);
  Statements then_stmts{};
  Statements else_stmts{};
  IfElseStatement node(cond, std::move(then_stmts), std::move(else_stmts));
  Statement *s = &node;
  EXPECT_NE(s, nullptr);
}

TEST(WhileStatementTest, EmptyBody) {
  auto *cond = new BoolLiteral(false);
  Statements body{};
  WhileStatement node(cond, std::move(body));
  EXPECT_NE(node.condition, nullptr);
  EXPECT_TRUE(node.body.empty());
}

TEST(WhileStatementTest, WithBody) {
  auto *cond = new BoolLiteral(true);
  Statements body{};
  body.push_back(
      std::make_unique<VariableDeclaration>("i", IntType{}, new IntLiteral(0)));
  WhileStatement node(cond, std::move(body));
  EXPECT_EQ(node.body.size(), 1u);
}

TEST(WhileStatementTest, IsStatement) {
  auto *cond = new BoolLiteral(false);
  Statements body{};
  WhileStatement node(cond, std::move(body));
  Statement *s = &node;
  EXPECT_NE(s, nullptr);
}

TEST(PrintStatementTest, StoresExpression) {
  auto *expr = new IntLiteral(77);
  PrintStatement stmt(expr);
  EXPECT_NE(stmt.expression, nullptr);
}

TEST(PrintStatementTest, IsStatement) {
  auto *expr = new BoolLiteral(true);
  PrintStatement stmt(expr);
  Statement *s = &stmt;
  EXPECT_NE(s, nullptr);
}

TEST(ProgramTest, EmptyProgram) {
  std::vector<std::unique_ptr<ClassDeclaration>> classes{};
  Statements stmts{};
  Program prog(std::move(classes), std::move(stmts));
  EXPECT_TRUE(prog.classes.empty());
  EXPECT_TRUE(prog.instructions.empty());
}

TEST(ProgramTest, ProgramWithStatements) {
  std::vector<std::unique_ptr<ClassDeclaration>> classes{};
  Statements stmts{};
  stmts.push_back(
      std::make_unique<VariableDeclaration>("x", IntType{}, new IntLiteral(0)));
  stmts.push_back(std::make_unique<PrintStatement>(new IdentityLiteral("x")));
  Program prog(std::move(classes), std::move(stmts));
  EXPECT_EQ(prog.instructions.size(), 2u);
}

TEST(ProgramTest, ProgramWithClass) {
  std::vector<std::unique_ptr<ClassDeclaration>> classes{};
  std::vector<FieldDeclaration> fields{{"x", IntType{}}};
  std::vector<std::unique_ptr<MethodDeclaration>> methods{};
  classes.push_back(std::make_unique<ClassDeclaration>("Foo", std::move(fields),
                                                       std::move(methods)));
  Statements stmts{};
  Program prog(std::move(classes), std::move(stmts));
  EXPECT_EQ(prog.classes.size(), 1u);
}

TEST(NestedExprTest, BinaryOfUnary) {
  auto *not_expr = new UnaryOperation(new BoolLiteral(true), "!");
  auto *one = new IntLiteral(1);
  BinaryOperation node(not_expr, one, "+");
  EXPECT_EQ(node.operation, "+");
  auto *unary = dynamic_cast<UnaryOperation *>(node.left_operand.get());
  EXPECT_NE(unary, nullptr);
  EXPECT_EQ(unary->operation, "!");
}

TEST(NestedExprTest, DeepNesting) {
  auto *sum = new BinaryOperation(new IntLiteral(1), new IntLiteral(2), "+");
  auto *diff = new BinaryOperation(new IntLiteral(3), new IntLiteral(4), "-");
  BinaryOperation prod(sum, diff, "*");
  EXPECT_EQ(prod.operation, "*");

  auto *left = dynamic_cast<BinaryOperation *>(prod.left_operand.get());
  auto *right = dynamic_cast<BinaryOperation *>(prod.right_operand.get());
  EXPECT_NE(left, nullptr);
  EXPECT_NE(right, nullptr);
  EXPECT_EQ(left->operation, "+");
  EXPECT_EQ(right->operation, "-");
}

TEST(ParserTest, EmptyFunction) {
  RunResult r = run_compiler("func main() -> int { }");
  EXPECT_EQ(r.exit_code, 0);
  EXPECT_EQ(r.err_output, "");
}

TEST(ParserTest, VarDeclarationInt) {
  RunResult r = run_compiler("func main() -> int { var x: int = 42; }");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, VarDeclarationBool) {
  RunResult r = run_compiler("func main() -> int { var flag: bool = true; }");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, AssignStatement) {
  RunResult r = run_compiler("func main() -> int { var x: int = 0; x = 5; }");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, PrintStatement) {
  RunResult r =
      run_compiler("func main() -> int { var x: int = 1; print(x); }");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, IfStatement) {
  RunResult r =
      run_compiler("func main() -> int { if (true) { var x: int = 1; } }");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, IfElseStatement) {
  RunResult r = run_compiler(
      "func main() -> int { if (true) { var x: int = 1; } else { var x: int = "
      "2; } }");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, ArithmeticExpression) {
  RunResult r = run_compiler("func main() -> int { var x: int = 2 + 3 * 4; }");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, BooleanExpression) {
  RunResult r = run_compiler(
      "func main() -> int { var b: bool = true && false || !true; }");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, ComparisonExpression) {
  RunResult r = run_compiler("func main() -> int { var b: bool = 1 < 2; }");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, ParenthesisedExpression) {
  RunResult r =
      run_compiler("func main() -> int { var x: int = (1 + 2) * (3 - 4); }");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, MultipleStatements) {
  RunResult r = run_compiler(R"(
        func main() -> int {
            var x: int = 1;
            var y: int = 2;
            var z: int = x + y;
            print(z);
        }
    )");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, NestedIf) {
  RunResult r = run_compiler(R"(
        func main() -> int {
            if (true) {
                if (false) {
                    var x: int = 1;
                }
            }
        }
    )");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, EmptyInputFails) {
  RunResult r = run_compiler("");
  EXPECT_NE(r.err_output.find("Error"), std::string::npos);
}

TEST(ParserTest, MissingSemicolonFails) {
  RunResult r = run_compiler("func main() -> int { var x: int = 1 }");
  EXPECT_NE(r.err_output.find("Error"), std::string::npos);
}

TEST(ParserTest, MissingClosingBraceFails) {
  RunResult r = run_compiler("func main() -> int { var x: int = 1;");
  EXPECT_NE(r.err_output.find("Error"), std::string::npos);
}

TEST(ParserTest, GarbageInputFails) {
  RunResult r = run_compiler("Garbage input");
  EXPECT_NE(r.err_output.find("Error"), std::string::npos);
}

TEST(ParserTest, MissingFuncKeywordFails) {
  RunResult r = run_compiler("main() -> int { }");
  EXPECT_NE(r.err_output.find("Error"), std::string::npos);
}

TEST(ParserTest, MissingArrowFails) {
  RunResult r = run_compiler("func main() int { }");
  EXPECT_NE(r.err_output.find("Error"), std::string::npos);
}

TEST(ParserTest, ElseIf) {
  RunResult r = run_compiler(R"(
    func main() -> int {
      var x: int = 1;
      if (x == 0) {
        print(0);
      } else if (x == 1) {
        print(1);
      } else {
        print(2);
      }
    }
  )");
  EXPECT_EQ(r.exit_code, 0);
  EXPECT_EQ(r.err_output, "");
}

TEST(ParserTest, ElseIfChain) {
  RunResult r = run_compiler(R"(
    func main() -> int {
      var x: int = 2;
      if (x == 0) {
        print(0);
      } else if (x == 1) {
        print(1);
      } else if (x == 2) {
        print(2);
      } else if (x == 3) {
        print(3);
      } else {
        print(4);
      }
    }
  )");
  EXPECT_EQ(r.exit_code, 0);
  EXPECT_EQ(r.err_output, "");
}

TEST(ParserTest, ElseIfWithoutFinalElse) {
  RunResult r = run_compiler(R"(
    func main() -> int {
      var x: int = 5;
      if (x == 0) {
        print(0);
      } else if (x == 1) {
        print(1);
      }
    }
  )");
  EXPECT_EQ(r.exit_code, 0);
  EXPECT_EQ(r.err_output, "");
}

TEST(InterpreterTest, ElseIfTakesCorrectBranch) {
  Interpreter interp;
  VariableDeclaration decl("x", IntType{}, new IntLiteral(0));
  decl.Accept(interp);

  Statements then1{};
  then1.push_back(std::make_unique<AssignStatement>("x", new IntLiteral(1)));

  Statements then2{};
  then2.push_back(std::make_unique<AssignStatement>("x", new IntLiteral(2)));

  Statements else2{};
  else2.push_back(std::make_unique<AssignStatement>("x", new IntLiteral(3)));

  Statements else1{};
  else1.push_back(std::make_unique<IfElseStatement>(
      new BoolLiteral(true), std::move(then2), std::move(else2)));

  IfElseStatement node(new BoolLiteral(false), std::move(then1),
                       std::move(else1));
  node.Accept(interp);
  EXPECT_EQ(interp.GetVar("x"), Interpreter::PossibleValue(2));
}

TEST(InterpreterTest, ElseIfFallsToFinalElse) {
  Interpreter interp;
  VariableDeclaration decl("x", IntType{}, new IntLiteral(0));
  decl.Accept(interp);

  Statements then1{};
  then1.push_back(std::make_unique<AssignStatement>("x", new IntLiteral(1)));

  Statements then2{};
  then2.push_back(std::make_unique<AssignStatement>("x", new IntLiteral(2)));

  Statements else2{};
  else2.push_back(std::make_unique<AssignStatement>("x", new IntLiteral(3)));

  Statements else1{};
  else1.push_back(std::make_unique<IfElseStatement>(
      new BoolLiteral(false), std::move(then2), std::move(else2)));

  IfElseStatement node(new BoolLiteral(false), std::move(then1),
                       std::move(else1));
  node.Accept(interp);
  EXPECT_EQ(interp.GetVar("x"), Interpreter::PossibleValue(3));
}

TEST(WhileStatementTest, WhileStatement) {
  RunResult r = run_compiler(
      "func main() -> int { var x: int = 0; while (x < 3) { x = x + 1; } }");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, WhileStatementEmptyBody) {
  RunResult r = run_compiler("func main() -> int { while (false) { } }");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, ClassDeclarationEmpty) {
  RunResult r = run_compiler("class Foo { } func main() -> int { }");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, ClassDeclarationWithField) {
  RunResult r =
      run_compiler("class Foo { var x: int; } func main() -> int { }");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, ClassDeclarationWithMethod) {
  RunResult r = run_compiler(
      "class Foo { func bar() -> int { } } func main() -> int { }");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, MethodCallStatement) {
  RunResult r = run_compiler(R"(
    class Foo { func bar() -> int { } }
    func main() -> int {
      var f: Foo = new Foo();
      f.bar();
    }
  )");
  EXPECT_EQ(r.exit_code, 0);
}

static std::string read_file(const std::string &path) {
  std::ifstream f(path);
  return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

TEST(PrintVisitorTest, IntLiteral) {
  {
    PrintVisitor v("/tmp/pv_test.txt");
    IntLiteral node(42);
    node.Accept(v);
  }
  EXPECT_NE(read_file("/tmp/pv_test.txt").find("IntLiteral: 42"),
            std::string::npos);
}

TEST(PrintVisitorTest, BoolLiteralTrue) {
  {
    PrintVisitor v("/tmp/pv_test.txt");
    BoolLiteral node(true);
    node.Accept(v);
  }
  EXPECT_NE(read_file("/tmp/pv_test.txt").find("BoolLiteral: true"),
            std::string::npos);
}

TEST(PrintVisitorTest, BoolLiteralFalse) {
  {
    PrintVisitor v("/tmp/pv_test.txt");
    BoolLiteral node(false);
    node.Accept(v);
  }
  EXPECT_NE(read_file("/tmp/pv_test.txt").find("BoolLiteral: false"),
            std::string::npos);
}

TEST(PrintVisitorTest, IdentityLiteral) {
  {
    PrintVisitor v("/tmp/pv_test.txt");
    IdentityLiteral node("myVar");
    node.Accept(v);
  }
  EXPECT_NE(read_file("/tmp/pv_test.txt").find("IdentityLiteral: myVar"),
            std::string::npos);
}

TEST(PrintVisitorTest, UnaryOperation) {
  {
    PrintVisitor v("/tmp/pv_test.txt");
    UnaryOperation node(new BoolLiteral(true), "!");
    node.Accept(v);
  }
  std::string out = read_file("/tmp/pv_test.txt");
  EXPECT_NE(out.find("UnaryOperation: !"), std::string::npos);
  EXPECT_NE(out.find("BoolLiteral: true"), std::string::npos);
}

TEST(PrintVisitorTest, BinaryOperation) {
  {
    PrintVisitor v("/tmp/pv_test.txt");
    BinaryOperation node(new IntLiteral(1), new IntLiteral(2), "+");
    node.Accept(v);
  }
  std::string out = read_file("/tmp/pv_test.txt");
  EXPECT_NE(out.find("BinaryOperation: +"), std::string::npos);
  EXPECT_NE(out.find("IntLiteral: 1"), std::string::npos);
  EXPECT_NE(out.find("IntLiteral: 2"), std::string::npos);
}

TEST(PrintVisitorTest, VariableDeclaration) {
  {
    PrintVisitor v("/tmp/pv_test.txt");
    VariableDeclaration node("x", IntType{}, new IntLiteral(5));
    node.Accept(v);
  }
  std::string out = read_file("/tmp/pv_test.txt");
  EXPECT_NE(out.find("Var x : int"), std::string::npos);
  EXPECT_NE(out.find("IntLiteral: 5"), std::string::npos);
}

TEST(PrintVisitorTest, IndentationIncreases) {
  {
    PrintVisitor v("/tmp/pv_test.txt");
    VariableDeclaration node("x", IntType{}, new IntLiteral(5));
    node.Accept(v);
  }
  std::string out = read_file("/tmp/pv_test.txt");
  size_t var_pos = out.find("Var");
  size_t int_pos = out.find("    IntLiteral");
  EXPECT_NE(var_pos, std::string::npos);
  EXPECT_NE(int_pos, std::string::npos);
  EXPECT_GT(int_pos, var_pos);
}

TEST(PrintVisitorTest, IfStatement) {
  {
    PrintVisitor v("/tmp/pv_test.txt");
    Statements stmts{};
    IfStatement node(new BoolLiteral(true), std::move(stmts));
    node.Accept(v);
  }
  std::string out = read_file("/tmp/pv_test.txt");
  EXPECT_NE(out.find("If"), std::string::npos);
  EXPECT_NE(out.find("Condition:"), std::string::npos);
  EXPECT_NE(out.find("Then:"), std::string::npos);
}

TEST(PrintVisitorTest, IfElseStatement) {
  {
    PrintVisitor v("/tmp/pv_test.txt");
    Statements then_stmts{};
    Statements else_stmts{};
    IfElseStatement node(new BoolLiteral(true), std::move(then_stmts),
                         std::move(else_stmts));
    node.Accept(v);
  }
  std::string out = read_file("/tmp/pv_test.txt");
  EXPECT_NE(out.find("Condition:"), std::string::npos);
  EXPECT_NE(out.find("Then:"), std::string::npos);
  EXPECT_NE(out.find("Else"), std::string::npos);
}

TEST(PrintVisitorTest, Program) {
  {
    PrintVisitor v("/tmp/pv_test.txt");
    std::vector<std::unique_ptr<ClassDeclaration>> classes{};
    Statements stmts{};
    stmts.push_back(std::make_unique<PrintStatement>(new IntLiteral(1)));
    Program prog(std::move(classes), std::move(stmts));
    prog.Accept(v);
  }
  std::string out = read_file("/tmp/pv_test.txt");
  EXPECT_NE(out.find("Program:"), std::string::npos);
  EXPECT_NE(out.find("Print:"), std::string::npos);
}

TEST(PrintVisitorTest, AssignStatement) {
  {
    PrintVisitor v("/tmp/pv_test.txt");
    AssignStatement node("x", new IntLiteral(7));
    node.Accept(v);
  }
  std::string out = read_file("/tmp/pv_test.txt");
  EXPECT_NE(out.find("Assign x"), std::string::npos);
  EXPECT_NE(out.find("IntLiteral: 7"), std::string::npos);
}

TEST(PrintVisitorTest, WhileStatement) {
  {
    PrintVisitor v("/tmp/pv_test.txt");
    Statements body{};
    WhileStatement node(new BoolLiteral(false), std::move(body));
    node.Accept(v);
  }
  std::string out = read_file("/tmp/pv_test.txt");
  EXPECT_NE(out.find("While"), std::string::npos);
  EXPECT_NE(out.find("Condition:"), std::string::npos);
  EXPECT_NE(out.find("Body:"), std::string::npos);
}

TEST(PrintVisitorTest, ClassDeclaration) {
  {
    PrintVisitor v("/tmp/pv_test.txt");
    std::vector<FieldDeclaration> fields{{"x", IntType{}}, {"y", BoolType{}}};
    std::vector<std::unique_ptr<MethodDeclaration>> methods{};
    ClassDeclaration node("MyClass", std::move(fields), std::move(methods));
    node.Accept(v);
  }
  std::string out = read_file("/tmp/pv_test.txt");
  EXPECT_NE(out.find("Class: MyClass"), std::string::npos);
  EXPECT_NE(out.find("Field x : int"), std::string::npos);
  EXPECT_NE(out.find("Field y : bool"), std::string::npos);
}

TEST(InterpreterTest, IntLiteral) {
  Interpreter interp;
  IntLiteral node(42);
  node.Accept(interp);
  EXPECT_EQ(interp.GetLastValue(), Interpreter::PossibleValue(42));
}

TEST(InterpreterTest, BoolLiteral) {
  Interpreter interp;
  BoolLiteral node(true);
  node.Accept(interp);
  EXPECT_EQ(interp.GetLastValue(), Interpreter::PossibleValue(true));
}

TEST(InterpreterTest, VariableDeclaration) {
  Interpreter interp;
  VariableDeclaration node("x", IntType{}, new IntLiteral(10));
  node.Accept(interp);
  EXPECT_EQ(interp.GetVar("x"), Interpreter::PossibleValue(10));
}

TEST(InterpreterTest, AssignStatement) {
  Interpreter interp;
  VariableDeclaration decl("x", IntType{}, new IntLiteral(1));
  decl.Accept(interp);
  AssignStatement assign("x", new IntLiteral(99));
  assign.Accept(interp);
  EXPECT_EQ(interp.GetVar("x"), Interpreter::PossibleValue(99));
}

TEST(InterpreterTest, IdentityLiteral) {
  Interpreter interp;
  VariableDeclaration decl("y", IntType{}, new IntLiteral(7));
  decl.Accept(interp);
  IdentityLiteral id("y");
  id.Accept(interp);
  EXPECT_EQ(interp.GetLastValue(), Interpreter::PossibleValue(7));
}

TEST(InterpreterTest, BinaryPlus) {
  Interpreter interp;
  BinaryOperation node(new IntLiteral(3), new IntLiteral(4), "+");
  node.Accept(interp);
  EXPECT_EQ(interp.GetLastValue(), Interpreter::PossibleValue(7));
}

TEST(InterpreterTest, BinaryMinus) {
  Interpreter interp;
  BinaryOperation node(new IntLiteral(10), new IntLiteral(3), "-");
  node.Accept(interp);
  EXPECT_EQ(interp.GetLastValue(), Interpreter::PossibleValue(7));
}

TEST(InterpreterTest, BinaryStar) {
  Interpreter interp;
  BinaryOperation node(new IntLiteral(3), new IntLiteral(4), "*");
  node.Accept(interp);
  EXPECT_EQ(interp.GetLastValue(), Interpreter::PossibleValue(12));
}

TEST(InterpreterTest, BinarySlash) {
  Interpreter interp;
  BinaryOperation node(new IntLiteral(10), new IntLiteral(2), "/");
  node.Accept(interp);
  EXPECT_EQ(interp.GetLastValue(), Interpreter::PossibleValue(5));
}

TEST(InterpreterTest, BinaryEqual) {
  Interpreter interp;
  BinaryOperation node(new IntLiteral(5), new IntLiteral(5), "==");
  node.Accept(interp);
  EXPECT_EQ(interp.GetLastValue(), Interpreter::PossibleValue(true));
}

TEST(InterpreterTest, BinaryNotEqual) {
  Interpreter interp;
  BinaryOperation node(new IntLiteral(5), new IntLiteral(3), "!=");
  node.Accept(interp);
  EXPECT_EQ(interp.GetLastValue(), Interpreter::PossibleValue(true));
}

TEST(InterpreterTest, BinaryLess) {
  Interpreter interp;
  BinaryOperation node(new IntLiteral(1), new IntLiteral(2), "<");
  node.Accept(interp);
  EXPECT_EQ(interp.GetLastValue(), Interpreter::PossibleValue(true));
}

TEST(InterpreterTest, BinaryGreater) {
  Interpreter interp;
  BinaryOperation node(new IntLiteral(5), new IntLiteral(2), ">");
  node.Accept(interp);
  EXPECT_EQ(interp.GetLastValue(), Interpreter::PossibleValue(true));
}

TEST(InterpreterTest, BinaryAnd) {
  Interpreter interp;
  BinaryOperation node(new BoolLiteral(true), new BoolLiteral(false), "&&");
  node.Accept(interp);
  EXPECT_EQ(interp.GetLastValue(), Interpreter::PossibleValue(false));
}

TEST(InterpreterTest, BinaryOr) {
  Interpreter interp;
  BinaryOperation node(new BoolLiteral(false), new BoolLiteral(true), "||");
  node.Accept(interp);
  EXPECT_EQ(interp.GetLastValue(), Interpreter::PossibleValue(true));
}

TEST(InterpreterTest, UnaryNot) {
  Interpreter interp;
  UnaryOperation node(new BoolLiteral(true), "!");
  node.Accept(interp);
  EXPECT_EQ(interp.GetLastValue(), Interpreter::PossibleValue(false));
}

TEST(InterpreterTest, UnaryNegate) {
  Interpreter interp;
  UnaryOperation node(new IntLiteral(5), "-");
  node.Accept(interp);
  EXPECT_EQ(interp.GetLastValue(), Interpreter::PossibleValue(-5));
}

TEST(InterpreterTest, IfTrueBranch) {
  Interpreter interp;
  VariableDeclaration decl("x", IntType{}, new IntLiteral(0));
  decl.Accept(interp);
  Statements stmts{};
  stmts.push_back(std::make_unique<AssignStatement>("x", new IntLiteral(1)));
  IfStatement node(new BoolLiteral(true), std::move(stmts));
  node.Accept(interp);
  EXPECT_EQ(interp.GetVar("x"), Interpreter::PossibleValue(1));
}

TEST(InterpreterTest, IfFalseBranchSkipped) {
  Interpreter interp;
  VariableDeclaration decl("x", IntType{}, new IntLiteral(0));
  decl.Accept(interp);
  Statements stmts{};
  stmts.push_back(std::make_unique<AssignStatement>("x", new IntLiteral(1)));
  IfStatement node(new BoolLiteral(false), std::move(stmts));
  node.Accept(interp);
  EXPECT_EQ(interp.GetVar("x"), Interpreter::PossibleValue(0));
}

TEST(InterpreterTest, IfElseThenBranch) {
  Interpreter interp;
  VariableDeclaration decl("x", IntType{}, new IntLiteral(0));
  decl.Accept(interp);
  Statements then_stmts{};
  then_stmts.push_back(
      std::make_unique<AssignStatement>("x", new IntLiteral(1)));
  Statements else_stmts{};
  else_stmts.push_back(
      std::make_unique<AssignStatement>("x", new IntLiteral(2)));
  IfElseStatement node(new BoolLiteral(true), std::move(then_stmts),
                       std::move(else_stmts));
  node.Accept(interp);
  EXPECT_EQ(interp.GetVar("x"), Interpreter::PossibleValue(1));
}

TEST(InterpreterTest, IfElseElseBranch) {
  Interpreter interp;
  VariableDeclaration decl("x", IntType{}, new IntLiteral(0));
  decl.Accept(interp);
  Statements then_stmts{};
  then_stmts.push_back(
      std::make_unique<AssignStatement>("x", new IntLiteral(1)));
  Statements else_stmts{};
  else_stmts.push_back(
      std::make_unique<AssignStatement>("x", new IntLiteral(2)));
  IfElseStatement node(new BoolLiteral(false), std::move(then_stmts),
                       std::move(else_stmts));
  node.Accept(interp);
  EXPECT_EQ(interp.GetVar("x"), Interpreter::PossibleValue(2));
}

TEST(InterpreterTest, PrintOutputsToStdout) {
  Interpreter interp;
  testing::internal::CaptureStdout();
  PrintStatement stmt(new IntLiteral(42));
  stmt.Accept(interp);
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_NE(output.find("42"), std::string::npos);
}

TEST(InterpreterTest, ComplexProgram) {
  Interpreter interp;
  std::vector<std::unique_ptr<ClassDeclaration>> classes{};
  Statements stmts{};
  stmts.push_back(std::make_unique<VariableDeclaration>(
      "x", IntType{},
      new BinaryOperation(new IntLiteral(2), new IntLiteral(3), "+")));
  stmts.push_back(std::make_unique<VariableDeclaration>(
      "y", IntType{},
      new BinaryOperation(new IdentityLiteral("x"), new IntLiteral(2), "*")));
  Program prog(std::move(classes), std::move(stmts));
  prog.Accept(interp);
  EXPECT_EQ(interp.GetVar("x"), Interpreter::PossibleValue(5));
  EXPECT_EQ(interp.GetVar("y"), Interpreter::PossibleValue(10));
}

TEST(InterpreterTest, WhileLoopNotEntered) {
  Interpreter interp;
  VariableDeclaration decl("x", IntType{}, new IntLiteral(0));
  decl.Accept(interp);
  Statements body{};
  body.push_back(std::make_unique<AssignStatement>("x", new IntLiteral(99)));
  WhileStatement node(new BoolLiteral(false), std::move(body));
  node.Accept(interp);
  EXPECT_EQ(interp.GetVar("x"), Interpreter::PossibleValue(0));
}

TEST(InterpreterTest, WhileLoopCountsDown) {
  Interpreter interp;
  VariableDeclaration decl("x", IntType{}, new IntLiteral(3));
  decl.Accept(interp);
  Statements body{};
  body.push_back(std::make_unique<AssignStatement>(
      "x",
      new BinaryOperation(new IdentityLiteral("x"), new IntLiteral(1), "-")));
  WhileStatement node(
      new BinaryOperation(new IdentityLiteral("x"), new IntLiteral(0), ">"),
      std::move(body));
  node.Accept(interp);
  EXPECT_EQ(interp.GetVar("x"), Interpreter::PossibleValue(0));
}

TEST(ScopeVisitorTest, UndeclaredVariable) {
  ScopeVisitor sv;
  IdentityLiteral node("undeclared");
  EXPECT_THROW(node.Accept(sv), std::runtime_error);
}

TEST(ScopeVisitorTest, DeclaredVariable) {
  ScopeVisitor sv;
  VariableDeclaration decl("x", IntType{}, new IntLiteral(1));
  decl.Accept(sv);
  IdentityLiteral use("x");
  EXPECT_NO_THROW(use.Accept(sv));
}

TEST(ScopeVisitorTest, DuplicateVariableSameScope) {
  ScopeVisitor sv;
  VariableDeclaration first("x", IntType{}, new IntLiteral(1));
  first.Accept(sv);
  VariableDeclaration second("x", IntType{}, new IntLiteral(2));
  EXPECT_THROW(second.Accept(sv), std::runtime_error);
}

TEST(ScopeVisitorTest, ShadowingAllowed) {
  ScopeVisitor sv;
  VariableDeclaration outer("x", IntType{}, new IntLiteral(1));
  outer.Accept(sv);
  Statements body{};
  body.push_back(
      std::make_unique<VariableDeclaration>("x", IntType{}, new IntLiteral(2)));
  IfStatement node(new BoolLiteral(true), std::move(body));
  EXPECT_NO_THROW(node.Accept(sv));
}

TEST(ScopeVisitorTest, ClassDeclaredTwice) {
  ScopeVisitor sv;
  std::vector<FieldDeclaration> f1{};
  std::vector<std::unique_ptr<MethodDeclaration>> m1{};
  ClassDeclaration first("Foo", std::move(f1), std::move(m1));
  first.Accept(sv);
  std::vector<FieldDeclaration> f2{};
  std::vector<std::unique_ptr<MethodDeclaration>> m2{};
  ClassDeclaration second("Foo", std::move(f2), std::move(m2));
  EXPECT_THROW(second.Accept(sv), std::runtime_error);
}

TEST(ScopeVisitorTest, UnknownClassInNew) {
  ScopeVisitor sv;
  NewObjectExpression node("UnknownClass");
  EXPECT_THROW(node.Accept(sv), std::runtime_error);
}

TEST(ScopeVisitorTest, MethodNotFound) {
  ScopeVisitor sv;
  std::vector<FieldDeclaration> fields{};
  std::vector<std::unique_ptr<MethodDeclaration>> methods{};
  ClassDeclaration cls("Bar", std::move(fields), std::move(methods));
  cls.Accept(sv);
  VariableDeclaration decl("b", ClassType{"Bar"},
                           new NewObjectExpression("Bar"));
  decl.Accept(sv);
  std::vector<std::unique_ptr<Expression>> args{};
  MethodCallExpression call("b", "nonexistent", std::move(args));
  EXPECT_THROW(call.Accept(sv), std::runtime_error);
}

TEST(ScopeVisitorTest, FieldNotFound) {
  ScopeVisitor sv;
  std::vector<FieldDeclaration> fields{};
  std::vector<std::unique_ptr<MethodDeclaration>> methods{};
  ClassDeclaration cls("Baz", std::move(fields), std::move(methods));
  cls.Accept(sv);
  VariableDeclaration decl("b", ClassType{"Baz"},
                           new NewObjectExpression("Baz"));
  decl.Accept(sv);
  FieldAccessExpression access("b", "noField");
  EXPECT_THROW(access.Accept(sv), std::runtime_error);
}

TEST(ScopeVisitorTest, WhileLoopScope) {
  ScopeVisitor sv;
  VariableDeclaration outer("i", IntType{}, new IntLiteral(0));
  outer.Accept(sv);
  Statements body{};
  body.push_back(
      std::make_unique<VariableDeclaration>("i", IntType{}, new IntLiteral(1)));
  WhileStatement node(new BoolLiteral(false), std::move(body));
  EXPECT_NO_THROW(node.Accept(sv));
}

TEST(CallableInfoTest, ConstructFromFunctionDeclaration) {
  std::vector<Parameter> params{{"x", IntType{}}, {"y", BoolType{}}};
  Statements body{};
  FunctionDeclaration func("add", std::move(params), IntType{},
                           std::move(body));

  CallableInfo ci(func);
  EXPECT_EQ(ci.name, "add");
  EXPECT_EQ(ci.return_type, Type{IntType{}});
  ASSERT_EQ(ci.params.size(), 2u);
  EXPECT_EQ(ci.params[0].name, "x");
  EXPECT_EQ(ci.params[0].type, Type{IntType{}});
  EXPECT_EQ(ci.params[1].name, "y");
  EXPECT_EQ(ci.params[1].type, Type{BoolType{}});
}

TEST(CallableInfoTest, ConstructFromMethodDeclaration) {
  std::vector<Parameter> params{{"n", IntType{}}};
  Statements body{};
  MethodDeclaration method("compute", std::move(params), BoolType{},
                           std::move(body));

  CallableInfo ci(method);
  EXPECT_EQ(ci.name, "compute");
  EXPECT_EQ(ci.return_type, Type{BoolType{}});
  ASSERT_EQ(ci.params.size(), 1u);
  EXPECT_EQ(ci.params[0].name, "n");
  EXPECT_EQ(ci.params[0].type, Type{IntType{}});
}

TEST(CallableInfoTest, NoParamsFunction) {
  Statements body{};
  FunctionDeclaration func("main", {}, IntType{}, std::move(body));

  CallableInfo ci(func);
  EXPECT_EQ(ci.name, "main");
  EXPECT_TRUE(ci.params.empty());
}

TEST(ClassInfoTest, ConstructFromClassDeclaration) {
  std::vector<FieldDeclaration> fields{{"x", IntType{}}, {"flag", BoolType{}}};
  std::vector<std::unique_ptr<MethodDeclaration>> methods{};
  Statements body{};
  methods.push_back(std::make_unique<MethodDeclaration>(
      "foo", std::vector<Parameter>{{"n", IntType{}}}, IntType{},
      std::move(body)));

  ClassDeclaration cls("MyClass", std::move(fields), std::move(methods));
  ClassInfo ci(cls);

  EXPECT_EQ(ci.name, "MyClass");
  ASSERT_EQ(ci.fields.size(), 2u);
  EXPECT_EQ(ci.fields.at("x"), Type{IntType{}});
  EXPECT_EQ(ci.fields.at("flag"), Type{BoolType{}});
  ASSERT_EQ(ci.methods.size(), 1u);
  EXPECT_EQ(ci.methods.at("foo").name, "foo");
  EXPECT_EQ(ci.methods.at("foo").return_type, Type{IntType{}});
  ASSERT_EQ(ci.methods.at("foo").params.size(), 1u);
  EXPECT_EQ(ci.methods.at("foo").params[0].name, "n");
}

TEST(ClassInfoTest, EmptyClass) {
  ClassDeclaration cls("Empty", {}, {});
  ClassInfo ci(cls);

  EXPECT_EQ(ci.name, "Empty");
  EXPECT_TRUE(ci.fields.empty());
  EXPECT_TRUE(ci.methods.empty());
}

TEST(ClassInfoTest, FindMethodExists) {
  std::vector<std::unique_ptr<MethodDeclaration>> methods{};
  Statements body{};
  methods.push_back(std::make_unique<MethodDeclaration>(
      "bar", std::vector<Parameter>{}, IntType{}, std::move(body)));
  ClassDeclaration cls("Foo", {}, std::move(methods));
  ClassInfo ci(cls);

  const CallableInfo *m = ci.FindMethod("bar");
  ASSERT_NE(m, nullptr);
  EXPECT_EQ(m->name, "bar");
}

TEST(ClassInfoTest, FindMethodNotExists) {
  ClassDeclaration cls("Foo", {}, {});
  ClassInfo ci(cls);
  EXPECT_EQ(ci.FindMethod("nonexistent"), nullptr);
}

TEST(ClassInfoTest, FindFieldExists) {
  std::vector<FieldDeclaration> fields{{"value", IntType{}}};
  ClassDeclaration cls("Foo", std::move(fields), {});
  ClassInfo ci(cls);

  const Type *f = ci.FindField("value");
  ASSERT_NE(f, nullptr);
  EXPECT_EQ(*f, Type{IntType{}});
}

TEST(ClassInfoTest, FindFieldNotExists) {
  ClassDeclaration cls("Foo", {}, {});
  ClassInfo ci(cls);
  EXPECT_EQ(ci.FindField("ghost"), nullptr);
}

TEST(VariableInfoTest, ConstructFromParameter) {
  Parameter p{"count", IntType{}};
  VariableInfo vi(p);
  EXPECT_EQ(vi.name, "count");
  EXPECT_EQ(vi.type, Type{IntType{}});
}

TEST(VariableInfoTest, ConstructFromFieldDeclaration) {
  FieldDeclaration f{"flag", BoolType{}};
  VariableInfo vi(f);
  EXPECT_EQ(vi.name, "flag");
  EXPECT_EQ(vi.type, Type{BoolType{}});
}

TEST(VariableInfoTest, ConstructFromVariableDeclaration) {
  VariableDeclaration decl("result", IntType{}, new IntLiteral(0));
  VariableInfo vi(decl);
  EXPECT_EQ(vi.name, "result");
  EXPECT_EQ(vi.type, Type{IntType{}});
}

TEST(SymbolTableTest, HasFunctionFalseInitially) {
  ScopeVisitor sv;
  EXPECT_FALSE(sv.GetSymbolTable().HasFunction("foo"));
}

TEST(SymbolTableTest, FunctionRegisteredAfterVisit) {
  ScopeVisitor sv;
  Statements body{};
  FunctionDeclaration func("foo", {}, IntType{}, std::move(body));
  func.Accept(sv);
  EXPECT_TRUE(sv.GetSymbolTable().HasFunction("foo"));
}

TEST(SymbolTableTest, GetFunctionReturnsNullForUnknown) {
  ScopeVisitor sv;
  EXPECT_EQ(sv.GetSymbolTable().GetFunction("unknown"), nullptr);
}

TEST(SymbolTableTest, GetFunctionReturnsCorrectInfo) {
  ScopeVisitor sv;
  std::vector<Parameter> params{{"x", IntType{}}, {"y", BoolType{}}};
  Statements body{};
  FunctionDeclaration func("add", std::move(params), IntType{},
                           std::move(body));
  func.Accept(sv);

  const CallableInfo *fi = sv.GetSymbolTable().GetFunction("add");
  ASSERT_NE(fi, nullptr);
  EXPECT_EQ(fi->name, "add");
  EXPECT_EQ(fi->return_type, Type{IntType{}});
  ASSERT_EQ(fi->params.size(), 2u);
  EXPECT_EQ(fi->params[0].name, "x");
  EXPECT_EQ(fi->params[1].name, "y");
}

TEST(SymbolTableTest, MultipleFunctionsRegistered) {
  ScopeVisitor sv;
  Statements b1{};
  FunctionDeclaration f1("foo", {}, IntType{}, std::move(b1));
  f1.Accept(sv);

  Statements b2{};
  FunctionDeclaration f2("bar", {{"n", IntType{}}}, BoolType{}, std::move(b2));
  f2.Accept(sv);

  EXPECT_TRUE(sv.GetSymbolTable().HasFunction("foo"));
  EXPECT_TRUE(sv.GetSymbolTable().HasFunction("bar"));
  EXPECT_EQ(sv.GetSymbolTable().GetFunction("bar")->params.size(), 1u);
}

TEST(SymbolTableTest, ClassAndFunctionIndependent) {
  ScopeVisitor sv;

  ClassDeclaration cls("Foo", {}, {});
  cls.Accept(sv);

  Statements body{};
  FunctionDeclaration func("foo", {}, IntType{}, std::move(body));
  func.Accept(sv);

  EXPECT_TRUE(sv.GetSymbolTable().HasClass("Foo"));
  EXPECT_TRUE(sv.GetSymbolTable().HasFunction("foo"));
  EXPECT_EQ(sv.GetSymbolTable().GetFunction("Foo"), nullptr);
  EXPECT_EQ(sv.GetSymbolTable().GetClass("foo"), nullptr);
}

TEST(ScopeVisitorTest, FunctionDeclaredTwice) {
  ScopeVisitor sv;
  Statements b1{};
  FunctionDeclaration f1("foo", {}, IntType{}, std::move(b1));
  f1.Accept(sv);

  Statements b2{};
  FunctionDeclaration f2("foo", {}, IntType{}, std::move(b2));
  EXPECT_THROW(f2.Accept(sv), std::runtime_error);
}

TEST(ScopeVisitorTest, UndefinedFunctionCallExpression) {
  ScopeVisitor sv;
  std::vector<std::unique_ptr<Expression>> args{};
  FunctionCallExpression call("unknown", std::move(args));
  EXPECT_THROW(call.Accept(sv), std::runtime_error);
}

TEST(ScopeVisitorTest, UndefinedFunctionCallStatement) {
  ScopeVisitor sv;
  std::vector<std::unique_ptr<Expression>> args{};
  FunctionCallStatement call("unknown", std::move(args));
  EXPECT_THROW(call.Accept(sv), std::runtime_error);
}

TEST(ScopeVisitorTest, FunctionCallExpressionAfterDeclarationOk) {
  ScopeVisitor sv;
  Statements body{};
  FunctionDeclaration func("compute", {}, IntType{}, std::move(body));
  func.Accept(sv);

  std::vector<std::unique_ptr<Expression>> args{};
  FunctionCallExpression call("compute", std::move(args));
  EXPECT_NO_THROW(call.Accept(sv));
}

TEST(ScopeVisitorTest, FunctionCallStatementAfterDeclarationOk) {
  ScopeVisitor sv;
  Statements body{};
  FunctionDeclaration func("greet", {}, IntType{}, std::move(body));
  func.Accept(sv);

  std::vector<std::unique_ptr<Expression>> args{};
  FunctionCallStatement call("greet", std::move(args));
  EXPECT_NO_THROW(call.Accept(sv));
}

TEST(ScopeVisitorTest, DuplicateParameterInFunction) {
  ScopeVisitor sv;
  std::vector<Parameter> params{{"x", IntType{}}, {"x", IntType{}}};
  Statements body{};
  FunctionDeclaration func("bad", std::move(params), IntType{},
                           std::move(body));
  EXPECT_THROW(func.Accept(sv), std::runtime_error);
}

TEST(ScopeVisitorTest, FunctionParametersVisibleInBody) {
  ScopeVisitor sv;
  std::vector<Parameter> params{{"x", IntType{}}};
  Statements body{};
  body.push_back(std::make_unique<PrintStatement>(new IdentityLiteral("x")));
  FunctionDeclaration func("f", std::move(params), IntType{}, std::move(body));
  EXPECT_NO_THROW(func.Accept(sv));
}

TEST(ScopeVisitorTest, FunctionParametersNotVisibleOutside) {
  ScopeVisitor sv;
  std::vector<Parameter> params{{"x", IntType{}}};
  Statements body{};
  FunctionDeclaration func("f", std::move(params), IntType{}, std::move(body));
  func.Accept(sv);

  IdentityLiteral use("x");
  EXPECT_THROW(use.Accept(sv), std::runtime_error);
}

TEST(ScopeVisitorTest, FunctionLocalVarsNotVisibleOutside) {
  ScopeVisitor sv;
  Statements body{};
  body.push_back(std::make_unique<VariableDeclaration>("local", IntType{},
                                                       new IntLiteral(0)));
  FunctionDeclaration func("f", {}, IntType{}, std::move(body));
  func.Accept(sv);

  IdentityLiteral use("local");
  EXPECT_THROW(use.Accept(sv), std::runtime_error);
}

TEST(ScopeVisitorTest, FunctionCanAccessGlobalVariable) {
  ScopeVisitor sv;
  VariableDeclaration global("g", IntType{}, new IntLiteral(42));
  global.Accept(sv);

  Statements body{};
  body.push_back(std::make_unique<PrintStatement>(new IdentityLiteral("g")));
  FunctionDeclaration func("f", {}, IntType{}, std::move(body));
  EXPECT_NO_THROW(func.Accept(sv));
}

TEST(ScopeVisitorTest, FunctionDoesNotConflictWithVariableName) {
  ScopeVisitor sv;
  VariableDeclaration global("g", IntType{}, new IntLiteral(0));
  global.Accept(sv);

  Statements body{};
  FunctionDeclaration func("g", {}, IntType{}, std::move(body));
  EXPECT_NO_THROW(func.Accept(sv));
}

TEST(InterpreterTest, SimpleFunctionCall) {
  Interpreter interp;

  std::vector<Parameter> params{{"a", IntType{}}, {"b", IntType{}}};
  Statements body{};
  body.push_back(std::make_unique<ReturnStatement>(new BinaryOperation(
      new IdentityLiteral("a"), new IdentityLiteral("b"), "+")));
  FunctionDeclaration func("add", std::move(params), IntType{},
                           std::move(body));
  func.Accept(interp);

  std::vector<std::unique_ptr<Expression>> args{};
  args.push_back(std::make_unique<IntLiteral>(3));
  args.push_back(std::make_unique<IntLiteral>(4));
  VariableDeclaration decl("result", IntType{},
                           new FunctionCallExpression("add", std::move(args)));
  decl.Accept(interp);

  EXPECT_EQ(interp.GetVar("result"), Interpreter::PossibleValue(7));
}

TEST(InterpreterTest, FunctionDoesNotSeeCallerLocals) {
  Interpreter interp;

  Statements body{};
  body.push_back(std::make_unique<ReturnStatement>(new IdentityLiteral("x")));
  FunctionDeclaration func("f", {}, IntType{}, std::move(body));
  func.Accept(interp);

  VariableDeclaration decl("x", IntType{}, new IntLiteral(10));
  decl.Accept(interp);

  std::vector<std::unique_ptr<Expression>> args{};
  FunctionCallExpression call("f", std::move(args));
  EXPECT_THROW(call.Accept(interp), std::out_of_range);
}

TEST(InterpreterTest, FunctionCallDoesNotMutateCallerScope) {
  Interpreter interp;

  std::vector<Parameter> params{{"x", IntType{}}};
  Statements body{};
  body.push_back(std::make_unique<AssignStatement>("x", new IntLiteral(99)));
  body.push_back(std::make_unique<ReturnStatement>(new IdentityLiteral("x")));
  FunctionDeclaration func("f", std::move(params), IntType{}, std::move(body));
  func.Accept(interp);

  VariableDeclaration decl("x", IntType{}, new IntLiteral(1));
  decl.Accept(interp);

  std::vector<std::unique_ptr<Expression>> args{};
  args.push_back(std::make_unique<IdentityLiteral>("x"));
  FunctionCallStatement call("f", std::move(args));
  call.Accept(interp);

  EXPECT_EQ(interp.GetVar("x"), Interpreter::PossibleValue(1));
}

TEST(InterpreterTest, RecursiveFunction) {
  Interpreter interp;

  std::vector<Parameter> params{{"n", IntType{}}};
  Statements body{};

  Statements base_case{};
  base_case.push_back(std::make_unique<ReturnStatement>(new IntLiteral(1)));
  body.push_back(std::make_unique<IfStatement>(
      new BinaryOperation(new IdentityLiteral("n"), new IntLiteral(1), "<="),
      std::move(base_case)));

  std::vector<std::unique_ptr<Expression>> rec_args{};
  rec_args.push_back(std::make_unique<BinaryOperation>(new IdentityLiteral("n"),
                                                       new IntLiteral(1), "-"));
  body.push_back(std::make_unique<ReturnStatement>(new BinaryOperation(
      new IdentityLiteral("n"),
      new FunctionCallExpression("factorial", std::move(rec_args)), "*")));

  FunctionDeclaration func("factorial", std::move(params), IntType{},
                           std::move(body));
  func.Accept(interp);

  std::vector<std::unique_ptr<Expression>> args{};
  args.push_back(std::make_unique<IntLiteral>(5));
  VariableDeclaration decl(
      "result", IntType{},
      new FunctionCallExpression("factorial", std::move(args)));
  decl.Accept(interp);

  EXPECT_EQ(interp.GetVar("result"), Interpreter::PossibleValue(120));
}

TEST(InterpreterTest, FunctionWithNoReturn) {
  Interpreter interp;
  Statements body{};
  FunctionDeclaration func("f", {}, IntType{}, std::move(body));
  func.Accept(interp);

  std::vector<std::unique_ptr<Expression>> args{};
  VariableDeclaration decl("result", IntType{},
                           new FunctionCallExpression("f", std::move(args)));
  decl.Accept(interp);

  EXPECT_EQ(interp.GetVar("result"), Interpreter::PossibleValue(0));
}

TEST(InterpreterTest, FunctionCallStatement) {
  Interpreter interp;

  Statements body{};
  FunctionDeclaration func("inc", {}, IntType{}, std::move(body));
  func.Accept(interp);

  std::vector<std::unique_ptr<Expression>> args{};
  FunctionCallStatement call("inc", std::move(args));
  EXPECT_NO_THROW(call.Accept(interp));
}

TEST(ParserTest, StandaloneFunctionDeclaration) {
  RunResult r = run_compiler(R"(
    func helper() -> int { }
    func main() -> int { }
  )");
  EXPECT_EQ(r.exit_code, 0);
  EXPECT_EQ(r.err_output, "");
}

TEST(ParserTest, FunctionWithParams) {
  RunResult r = run_compiler(R"(
    func add(a: int, b: int) -> int {
      return a + b;
    }
    func main() -> int { }
  )");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, FunctionCallAsExpression) {
  RunResult r = run_compiler(R"(
    func square(x: int) -> int {
      return x * x;
    }
    func main() -> int {
      var result: int = square(5);
      print(result);
    }
  )");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, FunctionCallAsStatement) {
  RunResult r = run_compiler(R"(
    func greet() -> int { }
    func main() -> int {
      greet();
    }
  )");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, MultipleFunctions) {
  RunResult r = run_compiler(R"(
    func foo() -> int { }
    func bar() -> int { }
    func baz() -> int { }
    func main() -> int { }
  )");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(ParserTest, FunctionWithClassesAndMethods) {
  RunResult r = run_compiler(R"(
    class Counter {
      var value: int;
      func increment() -> int { }
    }
    func reset() -> int { }
    func main() -> int { }
  )");
  EXPECT_EQ(r.exit_code, 0);
}

TEST(TypeCheckerTest, ValidTyping) {
  RunResult r = run_compiler(R"(
    func add(a: int, b: int) -> int {
      return a + b;
    }
    func main() -> int {
      var x: int = 10;
      var y: int = 20;
      var is_valid: bool = (x < y) && true;
      var res: int = add(x, y);

      var arr: int[] = new int[5];
      arr[0] = res;

      return 0;
    }
  )");
  EXPECT_EQ(r.exit_code, 0) << "Compiler error output:\n" << r.err_output;
}

TEST(TypeCheckerTest, VariableInitializationMismatch) {
  RunResult r = run_compiler(R"(
    func main() -> int {
      var x: int = true;
      return 0;
    }
  )");
  EXPECT_NE(r.exit_code, 0);
}

TEST(TypeCheckerTest, AssignmentMismatch) {
  RunResult r = run_compiler(R"(
    func main() -> int {
      var x: int = 5;
      x = false;
      return 0;
    }
  )");
  EXPECT_NE(r.exit_code, 0);
}

TEST(TypeCheckerTest, ArithmeticMismatch) {
  RunResult r = run_compiler(R"(
    func main() -> int {
      var x: int = 5 + true;
      return 0;
    }
  )");
  EXPECT_NE(r.exit_code, 0);
}

TEST(TypeCheckerTest, LogicalMismatch) {
  RunResult r = run_compiler(R"(
    func main() -> int {
      var b: bool = true && 5;
      return 0;
    }
  )");
  EXPECT_NE(r.exit_code, 0);
}

TEST(TypeCheckerTest, ConditionMismatchIf) {
  RunResult r = run_compiler(R"(
    func main() -> int {
      if (10) {
        print(1);
      }
      return 0;
    }
  )");
  EXPECT_NE(r.exit_code, 0);
}

TEST(TypeCheckerTest, FunctionArgTypeMismatch) {
  RunResult r = run_compiler(R"(
    func printInt(x: int) -> int { return 0; }
    func main() -> int {
      printInt(true);
      return 0;
    }
  )");
  EXPECT_NE(r.exit_code, 0);
}

TEST(TypeCheckerTest, FunctionArgCountMismatch) {
  RunResult r = run_compiler(R"(
    func sum(a: int, b: int) -> int { return a + b; }
    func main() -> int {
      sum(5);
      return 0;
    }
  )");
  EXPECT_NE(r.exit_code, 0);
}

TEST(TypeCheckerTest, ReturnTypeMismatch) {
  RunResult r = run_compiler(R"(
    func getBool() -> bool {
      return 5;
    }
    func main() -> int { return 0; }
  )");
  EXPECT_NE(r.exit_code, 0);
}

TEST(TypeCheckerTest, MethodCallOnNonObject) {
  RunResult r = run_compiler(R"(
    func main() -> int {
      var x: int = 5;
      x.doSomething();
      return 0;
    }
  )");
  EXPECT_NE(r.exit_code, 0);
}

TEST(TypeCheckerTest, ArrayAssignmentMismatch) {
  RunResult r = run_compiler(R"(
    func main() -> int {
      var arr: int[] = new int[5];
      arr[0] = true;
      return 0;
    }
  )");
  EXPECT_NE(r.exit_code, 0);
}

struct IrResult {
  bool compiled;
  bool linked;
  std::string output;
};

static IrResult run_ir(const std::string &src) {
  char src_file[] = "/tmp/ir_test_XXXXXX";
  int fd = mkstemp(src_file);
  write(fd, src.c_str(), src.size());
  close(fd);

  std::string bin = std::string(src_file) + ".out";

  std::string compile_cmd =
      std::string("./compiler -o ") + bin + " < " + src_file + " 2>/dev/null";
  int rc = system(compile_cmd.c_str());
  remove(src_file);
  remove((bin + ".o").c_str());
  if (WEXITSTATUS(rc) != 0)
    return {false, false, ""};

  if (access(bin.c_str(), X_OK) != 0)
    return {true, false, ""};

  FILE *pipe = popen(bin.c_str(), "r");
  std::string out;
  if (pipe) {
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe))
      out += buf;
    pclose(pipe);
  }
  remove(bin.c_str());
  return {true, true, out};
}

static std::vector<int> parse_ints(const std::string &s) {
  std::vector<int> result;
  std::istringstream ss(s);
  int n;
  while (ss >> n)
    result.push_back(n);
  return result;
}

TEST(IrTest, HelloWorld) {
  auto r = run_ir("func main() -> int { print(42); }");
  ASSERT_TRUE(r.compiled);
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{42}));
}

TEST(IrTest, PrintBoolTrue) {
  auto r = run_ir("func main() -> int { print(true); }");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{1}));
}

TEST(IrTest, PrintBoolFalse) {
  auto r = run_ir("func main() -> int { print(false); }");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{0}));
}

TEST(IrTest, ArithmeticAdd) {
  auto r = run_ir("func main() -> int { print(3 + 4); }");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{7}));
}

TEST(IrTest, ArithmeticSub) {
  auto r = run_ir("func main() -> int { print(10 - 3); }");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{7}));
}

TEST(IrTest, ArithmeticMul) {
  auto r = run_ir("func main() -> int { print(6 * 7); }");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{42}));
}

TEST(IrTest, ArithmeticDiv) {
  auto r = run_ir("func main() -> int { print(20 / 4); }");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{5}));
}

TEST(IrTest, ArithmeticNegation) {
  auto r = run_ir("func main() -> int { print(-7); }");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{-7}));
}

TEST(IrTest, ArithmeticPrecedence) {
  auto r = run_ir("func main() -> int { var x: int = 2 + 3 * 4; print(x); }");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{14}));
}

TEST(IrTest, LogicalNot) {
  auto r = run_ir("func main() -> int { print(!true); print(!false); }");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{0, 1}));
}

TEST(IrTest, LogicalAnd) {
  auto r = run_ir(R"(func main() -> int {
    print(true && true); print(true && false);
  })");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{1, 0}));
}

TEST(IrTest, LogicalOr) {
  auto r = run_ir(R"(func main() -> int {
    print(false || true); print(false || false);
  })");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{1, 0}));
}

TEST(IrTest, ComparisonLt) {
  auto r = run_ir("func main() -> int { print(1 < 2); print(2 < 1); }");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{1, 0}));
}

TEST(IrTest, ComparisonEq) {
  auto r = run_ir("func main() -> int { print(5 == 5); print(5 == 6); }");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{1, 0}));
}

TEST(IrTest, ComparisonNe) {
  auto r = run_ir("func main() -> int { print(5 != 6); print(5 != 5); }");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{1, 0}));
}

TEST(IrTest, ComparisonGe) {
  auto r = run_ir("func main() -> int { print(5 >= 5); print(4 >= 5); }");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{1, 0}));
}

TEST(IrTest, VariableDecl) {
  auto r = run_ir(R"(func main() -> int {
    var x: int = 10; var y: int = 20; print(x + y);
  })");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{30}));
}

TEST(IrTest, VariableReassign) {
  auto r =
      run_ir(R"(func main() -> int { var x: int = 1; x = 99; print(x); })");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{99}));
}

TEST(IrTest, IfTaken) {
  auto r = run_ir(R"(func main() -> int { if (true) { print(1); } })");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{1}));
}

TEST(IrTest, IfNotTaken) {
  auto r =
      run_ir(R"(func main() -> int { if (false) { print(1); } print(2); })");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{2}));
}

TEST(IrTest, IfElseThen) {
  auto r = run_ir(R"(func main() -> int {
    if (1 < 2) { print(100); } else { print(200); }
  })");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{100}));
}

TEST(IrTest, IfElseElse) {
  auto r = run_ir(R"(func main() -> int {
    if (2 < 1) { print(100); } else { print(200); }
  })");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{200}));
}

TEST(IrTest, IfElseIfChain) {
  auto r = run_ir(R"(
    func main() -> int {
      var x: int = 2;
      if (x == 1) { print(1); }
      else if (x == 2) { print(2); }
      else { print(3); }
    }
  )");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{2}));
}

TEST(IrTest, WhileSum) {
  auto r = run_ir(R"(
    func main() -> int {
      var i: int = 1; var s: int = 0;
      while (i <= 10) { s = s + i; i = i + 1; }
      print(s);
    }
  )");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{55}));
}

TEST(IrTest, WhileNotEntered) {
  auto r = run_ir(
      R"(func main() -> int { while (false) { print(99); } print(0); })");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{0}));
}

TEST(IrTest, FunctionAdd) {
  auto r = run_ir(R"(
    func add(a: int, b: int) -> int { return a + b; }
    func main() -> int { print(add(3, 4)); }
  )");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{7}));
}

TEST(IrTest, FunctionFactorial) {
  auto r = run_ir(R"(
    func factorial(n: int) -> int {
      if (n <= 1) { return 1; }
      return n * factorial(n - 1);
    }
    func main() -> int { print(factorial(6)); }
  )");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{720}));
}

TEST(IrTest, FunctionMultipleCalls) {
  auto r = run_ir(R"(
    func square(x: int) -> int { return x * x; }
    func main() -> int {
      print(square(3)); print(square(5)); print(square(10));
    }
  )");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{9, 25, 100}));
}

TEST(IrTest, FunctionReturnBool) {
  auto r = run_ir(R"(
    func isPositive(n: int) -> bool {
      if (n > 0) { return true; }
      return false;
    }
    func main() -> int { print(isPositive(5)); print(isPositive(-1)); }
  )");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{1, 0}));
}

TEST(IrTest, ClassFieldsZeroInit) {
  auto r = run_ir(R"(
    class Point { var x: int; var y: int; }
    func main() -> int {
      var p: Point = new Point();
      print(p.x); print(p.y);
    }
  )");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{0, 0}));
}

TEST(IrTest, ClassMethodCall) {
  auto r = run_ir(R"(
    class Counter {
      var count: int;
      func increment(step: int) -> int {
        count = count + step;
        return count;
      }
    }
    func main() -> int {
      var c: Counter = new Counter();
      print(c.increment(1));
      print(c.increment(4));
      print(c.increment(10));
    }
  )");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{1, 5, 15}));
}

TEST(IrTest, ClassMethodReturnBool) {
  auto r = run_ir(R"(
    class Checker {
      func isPositive(n: int) -> bool {
        if (n > 0) { return true; }
        return false;
      }
    }
    func main() -> int {
      var c: Checker = new Checker();
      print(c.isPositive(5)); print(c.isPositive(-3));
    }
  )");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{1, 0}));
}

TEST(IrTest, ClassTwoInstancesIndependent) {
  auto r = run_ir(R"(
    class Box {
      var val: int;
      func set(v: int) -> int { val = v; return val; }
      func get() -> int { return val; }
    }
    func main() -> int {
      var a: Box = new Box();
      var b: Box = new Box();
      a.set(10); b.set(20);
      print(a.get()); print(b.get());
    }
  )");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{10, 20}));
}

TEST(IrTest, ArrayReadWrite) {
  auto r = run_ir(R"(
    func main() -> int {
      var arr: int[] = new int[3];
      arr[0] = 10; arr[1] = 20; arr[2] = 30;
      print(arr[0]); print(arr[1]); print(arr[2]);
    }
  )");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{10, 20, 30}));
}

TEST(IrTest, ArraySumLoop) {
  auto r = run_ir(R"(
    func main() -> int {
      var arr: int[] = new int[5];
      arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 4; arr[4] = 5;
      var i: int = 0; var s: int = 0;
      while (i < 5) { s = s + arr[i]; i = i + 1; }
      print(s);
    }
  )");
  ASSERT_TRUE(r.linked);
  EXPECT_EQ(parse_ints(r.output), (std::vector<int>{15}));
}

TEST(IrTest, AllExamplesCompileAndLink) {
  for (int i = 1; i <= 15; ++i) {
    char glob[64];
    snprintf(glob, sizeof(glob), "../examples/%02d_*.txt", i);
    FILE *ls = popen((std::string("ls ") + glob + " 2>/dev/null").c_str(), "r");
    char path[256] = {};
    if (ls) {
      fgets(path, sizeof(path), ls);
      pclose(ls);
    }
    for (char *c = path; *c; ++c)
      if (*c == '\n') {
        *c = 0;
        break;
      }
    if (!path[0])
      continue;

    std::ifstream f(path);
    std::string src(std::istreambuf_iterator<char>(f), {});
    auto r = run_ir(src);
    EXPECT_TRUE(r.compiled) << "compiler failed: " << path;
    EXPECT_TRUE(r.linked) << "executable not produced: " << path;
  }
}

static std::optional<std::string> compile_and_run(const std::string &src) {
  auto r = run_ir(src);
  if (!r.compiled || !r.linked)
    return std::nullopt;
  return r.output;
}

TEST(CompilerTest, ProducesExecutable) {
  auto r = run_ir("func main() -> int { print(1); }");
  EXPECT_TRUE(r.compiled) << "compiler returned non-zero";
  EXPECT_TRUE(r.linked)   << "executable was not produced";
}

TEST(CompilerTest, OutputMatchesInterpreter) {
  auto out = compile_and_run("func main() -> int { print(2 + 2); }");
  ASSERT_TRUE(out.has_value());
  EXPECT_EQ(parse_ints(*out), (std::vector<int>{4}));
}

TEST(CompilerTest, OptimizationsDoNotBreakArithmetic) {
  auto out = compile_and_run(R"(
    func main() -> int {
      var x: int = 10 * 10 + 1;
      print(x);
    }
  )");
  ASSERT_TRUE(out.has_value());
  EXPECT_EQ(parse_ints(*out), (std::vector<int>{101}));
}

TEST(CompilerTest, OptimizationsDoNotBreakLoop) {
  auto out = compile_and_run(R"(
    func main() -> int {
      var i: int = 0;
      while (i < 3) {
        print(i);
        i = i + 1;
      }
    }
  )");
  ASSERT_TRUE(out.has_value());
  EXPECT_EQ(parse_ints(*out), (std::vector<int>{0, 1, 2}));
}

TEST(CompilerTest, OptimizationsDoNotBreakConditional) {
  auto out = compile_and_run(R"(
    func main() -> int {
      var x: int = 5;
      if (x > 3) {
        print(1);
      } else {
        print(0);
      }
    }
  )");
  ASSERT_TRUE(out.has_value());
  EXPECT_EQ(parse_ints(*out), (std::vector<int>{1}));
}

TEST(CompilerTest, RecursiveFunctionCompilesAndRuns) {
  auto out = compile_and_run(R"(
    func fact(n: int) -> int {
      if (n <= 1) { return 1; }
      return n * fact(n - 1);
    }
    func main() -> int {
      print(fact(6));
    }
  )");
  ASSERT_TRUE(out.has_value());
  EXPECT_EQ(parse_ints(*out), (std::vector<int>{720}));
}

TEST(CompilerTest, ClassFieldsCompiledCorrectly) {
  auto out = compile_and_run(R"(
    class Counter {
      var value: int;
      func set(n: int) -> int { value = n; return 0; }
      func get() -> int { return value; }
      func add(n: int) -> int { value = value + n; return 0; }
    }
    func main() -> int {
      var c: Counter = new Counter();
      c.set(3);
      c.add(4);
      print(c.get());
    }
  )");
  ASSERT_TRUE(out.has_value());
  EXPECT_EQ(parse_ints(*out), (std::vector<int>{7}));
}

TEST(CompilerTest, InvalidSourceDoesNotProduceExecutable) {
  auto r = run_ir("this is not valid source !!!");
  EXPECT_FALSE(r.linked);
}

TEST(CompilerTest, MultipleOutputCallsAreIndependent) {
  auto out1 = compile_and_run("func main() -> int { print(111); }");
  auto out2 = compile_and_run("func main() -> int { print(222); }");
  ASSERT_TRUE(out1.has_value());
  ASSERT_TRUE(out2.has_value());
  EXPECT_EQ(parse_ints(*out1), (std::vector<int>{111}));
  EXPECT_EQ(parse_ints(*out2), (std::vector<int>{222}));
}

TEST(CompilerTest, AllExamplesBuildToNativeBinary) {
  for (int i = 1; i <= 15; ++i) {
    char glob[64];
    snprintf(glob, sizeof(glob), "../examples/%02d_*.txt", i);
    FILE *ls = popen((std::string("ls ") + glob + " 2>/dev/null").c_str(), "r");
    char path[256] = {};
    if (ls) {
      fgets(path, sizeof(path), ls);
      pclose(ls);
    }
    for (char *c = path; *c; ++c)
      if (*c == '\n') { *c = 0; break; }
    if (!path[0]) continue;

    std::ifstream f(path);
    std::string src(std::istreambuf_iterator<char>(f), {});
    auto r = run_ir(src);
    EXPECT_TRUE(r.compiled) << "compiler failed: " << path;
    EXPECT_TRUE(r.linked)   << "native binary not produced: " << path;
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}