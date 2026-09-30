%{
  #include <string>
  #include "parser.tab.hh"
  using token = yy::parser::token;
  #define YYSTYPE yy::parser::value_type
%}

%option noyywrap
%option bison-bridge

%%

"func" { return token::FUNC; }
"->" { return token::ARROW; }
"print" { return token::PRINT; }
"var" { return token::VAR; }
"struct" { return token::STRUCT; }
"new" { return token::NEW; }

"bool" { return token::TYPE_BOOL; }
"int" { return token::TYPE_INT; }

"if" { return token::IF; }
"else" { return token::ELSE; }
"while" { return token::WHILE; }
"return" { return token::RETURN; }

"+" { return token::PLUS; }
"-" { return token::MINUS; }
"*" { return token::STAR; }
"/" { return token::SLASH; }
"(" { return token::LPAREN; }
")" { return token::RPAREN; }
"{" { return token::LBRACE; }
"}" { return token::RBRACE; }
"[" { return token::LBRACKET; }
"]" { return token::RBRACKET; }
"==" { return token::EQUAL; }
"!=" { return token::UNEQUAL; }
"=" { return token::ASSIGN; }
"!" { return token::NOT; }
">" { return token::GREATER; }
"<" { return token::LESS; }
">=" { return token::GEQUAL; }
"<=" { return token::LEQUAL; }
"&&" { return token::AND; }
"||" { return token::OR; }
":" { return token::COLON; }
";" { return token::SEMICOLON; }
"." { return token::DOT; }
"," { return token::COMMA; }

"true" {
  yylval_param->emplace<bool>(true);
  return token::BOOL_LITERAL;
}

"false" {
  yylval_param->emplace<bool>(false);
  return token::BOOL_LITERAL;
}

[0-9]+ {
  yylval_param->emplace<int>(atoi(yytext));
  return token::INT_LITERAL;
}

[a-zA-Z_][a-zA-Z0-9_]* {
  yylval_param->emplace<std::string>(yytext);
  return token::IDENTITY;
}

[ \t\n\r]+ {
  /* Будем пропускать различного рода пробелы */
}


%%
