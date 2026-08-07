%code requires {
  #include <string>
  #include <vector>
  #include <memory>
  #include "tree.hpp"
}

%skeleton "lalr1.cc"
%define api.value.type variant

%parse-param { Program*& root }

%code {
  extern int yylex(yy::parser::value_type*);
}

%token FUNC ARROW CLASS NEW
%token VAR IF ELSE PRINT WHILE RETURN
%token EQUAL UNEQUAL
%token PLUS MINUS STAR SLASH
%token ASSIGN NOT
%token LESS GREATER LEQUAL GEQUAL
%token LPAREN RPAREN LBRACE RBRACE LBRACKET RBRACKET
%token COLON SEMICOLON DOT COMMA
%token TYPE_BOOL TYPE_INT

%token <int> INT_LITERAL
%token <bool> BOOL_LITERAL
%token <std::string> IDENTITY

%type <Expression*> expression
%type <Statement*> statement
%type <std::vector<std::unique_ptr<Statement>>> statements else_branch
%type <Type> type

%type <std::vector<std::unique_ptr<ClassDeclaration>>> class_list
%type <ClassDeclaration*> class_declaration
%type <std::vector<FieldDeclaration>> field_list
%type <std::vector<std::unique_ptr<MethodDeclaration>>> method_list
%type <MethodDeclaration*> method_declaration
%type <std::vector<std::unique_ptr<FunctionDeclaration>>> function_list
%type <FunctionDeclaration*> function_declaration
%type <std::vector<Parameter>> params param_list
%type <std::vector<std::unique_ptr<Expression>>> args arg_list

%right ASSIGN
%left OR
%left AND
%nonassoc EQUAL UNEQUAL
%nonassoc LESS GREATER LEQUAL GEQUAL
%left PLUS MINUS
%left STAR SLASH
%precedence NOT

%%

program:
  class_list function_list {
    Statements main_body;
    std::vector<std::unique_ptr<FunctionDeclaration>> functions;

    for (auto& function : $2) {
      if (function->name == "main" && function->parameters.empty()) {
        main_body = std::move(function->body);
      } else {
        functions.push_back(std::move(function));
      }
    }

    root = new Program(std::move($1), std::move(functions), std::move(main_body));
  }
;

function_list:
  function_declaration {
    $$ = std::vector<std::unique_ptr<FunctionDeclaration>>{};
    $$.push_back(std::unique_ptr<FunctionDeclaration>($1));
  }
| function_list function_declaration {
    $1.push_back(std::unique_ptr<FunctionDeclaration>($2));
    $$ = std::move($1);
  }
;

function_declaration:
  FUNC IDENTITY LPAREN params RPAREN ARROW type LBRACE statements RBRACE {
    $$ = new FunctionDeclaration($2, std::move($4), $7, std::move($9));
  }
;

class_list:
  %empty {
    $$ = std::vector<std::unique_ptr<ClassDeclaration>>{};
  }
| class_list class_declaration {
    $1.push_back(std::unique_ptr<ClassDeclaration>($2));
    $$ = std::move($1);
  }
;

class_declaration:
  CLASS IDENTITY LBRACE field_list method_list RBRACE {
    $$ = new ClassDeclaration($2, std::move($4), std::move($5));
  }
;

field_list:
  %empty {
    $$ = std::vector<FieldDeclaration>{};
  }
| field_list VAR IDENTITY COLON type SEMICOLON {
    $1.push_back(FieldDeclaration(std::move($3), std::move($5)));
    $$ = std::move($1);
  }
;

method_list:
  %empty {
    $$ = std::vector<std::unique_ptr<MethodDeclaration>>{};
  }
| method_list method_declaration {
    $1.push_back(std::unique_ptr<MethodDeclaration>($2));
    $$ = std::move($1);
  }
;

method_declaration:
  FUNC IDENTITY LPAREN params RPAREN ARROW type LBRACE statements RBRACE {
    $$ = new MethodDeclaration($2, std::move($4), $7, std::move($9));
  }
;

params:
  %empty {
    $$ = std::vector<Parameter>{};
  }
| param_list {
    $$ = std::move($1);
  }
;

param_list:
  IDENTITY COLON type {
    $$ = std::vector<Parameter>{Parameter(std::move($1), std::move($3))};
  }
| param_list COMMA IDENTITY COLON type {
    $1.push_back(Parameter(std::move($3), std::move($5)));
    $$ = std::move($1);
  }
;

type:
  TYPE_INT {
    $$ = IntType{};
  }
| TYPE_BOOL {
    $$ = BoolType{};
  }
| IDENTITY {
    $$ = ClassType{$1};
  }
| type LBRACKET RBRACKET {
    $$ = ArrayType{std::make_unique<Type>($1)};
  }
;

statements:
  %empty {
    $$ = std::vector<std::unique_ptr<Statement>>{};
  }
| statements statement {
    $1.push_back(std::unique_ptr<Statement>($2));
    $$ = std::move($1);
  }
;

else_branch:
  LBRACE statements RBRACE {
    $$ = std::move($2);
  }
| IF LPAREN expression RPAREN LBRACE statements RBRACE {
    Statements tmp;
    tmp.push_back(std::unique_ptr<Statement>(new IfStatement($3, std::move($6))));
    $$ = std::move(tmp);
  }
| IF LPAREN expression RPAREN LBRACE statements RBRACE ELSE else_branch {
    Statements tmp;
    tmp.push_back(std::unique_ptr<Statement>(new IfElseStatement($3, std::move($6), std::move($9))));
    $$ = std::move(tmp);
  }
;

statement:
  VAR IDENTITY COLON type ASSIGN expression SEMICOLON {
    $$ = new VariableDeclaration(std::move($2), std::move($4), $6);
  }
| IDENTITY ASSIGN expression SEMICOLON {
    $$ = new AssignStatement($1, $3);
  }
| IF LPAREN expression RPAREN LBRACE statements RBRACE {
    $$ = new IfStatement($3, std::move($6));
  }
| IF LPAREN expression RPAREN LBRACE statements RBRACE ELSE else_branch {
    $$ = new IfElseStatement($3, std::move($6), std::move($9));
  }
| PRINT LPAREN expression RPAREN SEMICOLON {
    $$ = new PrintStatement($3);
  }
| WHILE LPAREN expression RPAREN LBRACE statements RBRACE {
    $$ = new WhileStatement($3, std::move($6));
  }
| RETURN expression SEMICOLON {
    $$ = new ReturnStatement($2);
  }
| IDENTITY DOT IDENTITY LPAREN args RPAREN SEMICOLON {
    $$ = new MethodCallStatement($1, $3, std::move($5));
  }
| IDENTITY LPAREN args RPAREN SEMICOLON {
    $$ = new FunctionCallStatement($1, std::move($3));
  }
| IDENTITY LBRACKET expression RBRACKET ASSIGN expression SEMICOLON {
    $$ = new ArrayAssignStatement($1, $3, $6);
  }
;

expression:
  INT_LITERAL  {
    $$ = new IntLiteral($1);
  }
| BOOL_LITERAL {
    $$ = new BoolLiteral($1);
  }
| IDENTITY {
    $$ = new IdentityLiteral($1);
  }
| expression PLUS expression {
    $$ = new BinaryOperation($1, $3, "+");
  }
| expression MINUS expression {
    $$ = new BinaryOperation($1, $3, "-");
  }
| expression STAR expression {
    $$ = new BinaryOperation($1, $3, "*");
  }
| expression SLASH expression {
    $$ = new BinaryOperation($1, $3, "/");
  }
| expression EQUAL expression {
    $$ = new BinaryOperation($1, $3, "==");
  }
| expression UNEQUAL expression {
    $$ = new BinaryOperation($1, $3, "!=");
  }
| expression LESS expression {
    $$ = new BinaryOperation($1, $3, "<");
  }
| expression GREATER expression {
    $$ = new BinaryOperation($1, $3, ">");
  }
| expression LEQUAL expression {
    $$ = new BinaryOperation($1, $3, "<=");
  }
| expression GEQUAL expression {
    $$ = new BinaryOperation($1, $3, ">=");
  }
| expression AND expression {
    $$ = new BinaryOperation($1, $3, "&&");
  }
| expression OR expression {
    $$ = new BinaryOperation($1, $3, "||");
  }
| NOT expression {
    $$ = new UnaryOperation($2, "!");
  }
| MINUS expression {
    $$ = new UnaryOperation($2, "-");
  }
| LPAREN expression RPAREN {
    $$ = $2;
  }
| NEW IDENTITY LPAREN RPAREN {
    $$ = new NewObjectExpression($2);
  }
| NEW type LBRACKET expression RBRACKET {
    $$ = new NewArrayExpression($2, $4);
  }
| IDENTITY DOT IDENTITY LPAREN args RPAREN {
    $$ = new MethodCallExpression($1, $3, std::move($5));
  }
| IDENTITY LPAREN args RPAREN {
    $$ = new FunctionCallExpression($1, std::move($3));
  }
| IDENTITY DOT IDENTITY {
    $$ = new FieldAccessExpression($1, $3);
  }
| IDENTITY LBRACKET expression RBRACKET {
    $$ = new ArrayIndexExpression($1, $3);
  }
;

args:
  %empty   {
    $$ = std::vector<std::unique_ptr<Expression>>{};
  }
| arg_list {
    $$ = std::move($1);
  }
;

arg_list:
  expression {
    std::vector<std::unique_ptr<Expression>> v;
    v.push_back(std::unique_ptr<Expression>($1));
    $$ = std::move(v);
  }
| arg_list COMMA expression {
    $1.push_back(std::unique_ptr<Expression>($3));
    $$ = std::move($1);
  }
;

%%

void yy::parser::error(const std::string& msg) {
  fprintf(stderr, "Error: %s\n", msg.c_str());
}
