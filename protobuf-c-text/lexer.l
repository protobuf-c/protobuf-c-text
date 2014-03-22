%{
#include <stdio.h>
#include <string.h>
#include "lexer-global.h"
#include "parser.h"
%}
%%
-?[0-9]+|-?[0-9]*\.[0-9]+  yylval.string=strdup(yytext); return NUMBER;
true                       yylval.boolean=true; return BOOLEAN;
false                      yylval.boolean=false; return BOOLEAN;
[a-zA-Z0-9_]+              yylval.string=strdup(yytext); return BAREWORD;
\"([^\\\"]|\\.)*\"         {
                              yylval.string=strdup(yytext + 1);
                              yylval.string[strlen(yylval.string) - 1] = '\0';
                              return QUOTED;
                           }
[{]                        yylval.string = "{"; return OBRACE;
[}]                        yylval.string = "}"; return CBRACE;
:                          yylval.string = ":"; return COLON;
[ \t\n]                    /* Ignore whitespace */
%%
