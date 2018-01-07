%{

#include "lyutils.h"
#include "auxlib.h"

#define YY_USER_ACTION  { lexer::advance(); }



%}
%option 8bit
%option debug
%option ecs
%option nodefault
%option nounput
%option noyywrap
%option perf-report
%option verbose
%option warn

LETTER          [A-Za-z_]
DIGIT           -?[0-9]
STRING          (\"([^\\"\n]|\\[\\'"0nt])*\")
INTEGER         ({DIGIT}+)
NUMBER          ({INTEGER})
IDENT           ({LETTER}({LETTER}|{DIGIT})*)
NOTIDENT        (({LETTER}|{DIGIT})*)
NOTSTRING       (\"([^\\"\n]|\\.)*(\")?)

%%

"#".*           { lexer::include(); }
[ \t]+          { }
\n              { lexer::newline(); }

void            { return yylval_token(TOK_VOID); }
int             { return yylval_token(TOK_INT); }
string          { return yylval_token(TOK_STRING); }
struct          { return yylval_token(TOK_STRUCT); }
if              { return yylval_token(TOK_IF); }
else            { return yylval_token(TOK_ELSE); }
while           { return yylval_token(TOK_WHILE); }
return          { return yylval_token(TOK_RETURN); }
new             { return yylval_token(TOK_NEW); }
null            { return yylval_token(TOK_NULL); }


{NUMBER}        { return yylval_token (TOK_INTCON); }
{IDENT}         { return yylval_token (TOK_IDENT); }
(\[\])          { return yylval_token (TOK_ARRAY); } 
{STRING}        { return yylval_token (TOK_STRINGCON); }


"("             { return yylval_token ('('); }
"'"             { return yylval_token ('\''); }
")"             { return yylval_token (')'); }
"["             { return yylval_token ('['); }
"]"             { return yylval_token (']'); }
"{"             { return yylval_token ('{'); }
"}"             { return yylval_token ('}'); }
";"             { return yylval_token (';'); }
","             { return yylval_token (','); }
"."             { return yylval_token ('.'); }
"="             { return yylval_token ('='); }
"=="            { return yylval_token (TOK_EQ); }
"!="            { return yylval_token (TOK_NE); }
"<"             { return yylval_token (TOK_LT); }
"<="            { return yylval_token (TOK_LE); }
">"             { return yylval_token (TOK_GT); }
">="            { return yylval_token (TOK_GE); }
"+"             { return yylval_token ('+'); }
"-"             { return yylval_token ('-'); }
"*"             { return yylval_token ('*'); }
"/"             { return yylval_token ('/'); }
"%"             { return yylval_token ('%'); }
"!"             { return yylval_token ('!'); }


{NOTIDENT}      { lexer::badtoken (yytext);
                  return yylval_token (TOK_IDENT); }

{NOTSTRING}     { lexer::badtoken (yytext);
                  return yylval_token (TOK_STRINGCON); }

.               { lexer::badchar (*yytext); }

%%


