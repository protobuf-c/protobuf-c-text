/*
 * vim: set filetype=c:
 *
 * Compile with: re2c -s -o protobuf-c-text/lexer.re.c protobuf-c-text/lexer.re
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "lexer-global.h"
#include "parser.h"

typedef struct _Scanner {
  unsigned char *cursor;
  unsigned char *buffer;
  unsigned char *limit;
  unsigned char *token;
  bool boolean;
  FILE *f;
} Scanner;

int
fill(Scanner *s, unsigned char *cursor)
{
  return s->limit >= cursor;
}

#define YYFILL(n) { if (!fill(s, cursor)) return 0; }
#define RETURN(t) { s->cursor = cursor; return t; }

int
scan(Scanner *s)
{
  unsigned char *cursor;

  cursor = s->cursor;
token_start:
  s->token = cursor;

  /*!re2c
  re2c:define:YYCTYPE   = "unsigned char";
  re2c:define:YYCURSOR  = cursor;
  re2c:define:YYLIMIT   = s->limit;
  re2c:define:YYMARKER  = s->buffer;

  I = [-]? [0-9]+;
  F = [-]? [0-9]* "." [0-9]+;
  BW = [a-zA-Z0-9_]+;
  EQ = [\\] ["];
  NL = "\n";
  QS = ["] (EQ|[^"]|NL)* ["];
  WS = [ \t\n];

  I | F           { RETURN(NUMBER); }
  "true"          { s->boolean=true; RETURN(BOOLEAN); }
  "false"         { s->boolean=false; RETURN(BOOLEAN); }
  BW              { RETURN(BAREWORD); }
  QS              { RETURN(QUOTED); }
  "{"             { RETURN(OBRACE); }
  "}"             { RETURN(CBRACE); }
  ":"             { RETURN(COLON); }
  WS              { goto token_start; }
  "\000"          { return 0; }
  */
}

int main() {
  Scanner in;
  int t;

  in.buffer = "x { moo: true boo: 5 bob: \"moo cow\" }\n\"moo";
  in.cursor = in.buffer;
  in.limit = &in.buffer[strlen(in.buffer)];
  while (t = scan(&in)) {
    printf("%d: %.*s\n", t, (int)(in.cursor - in.token), in.token);
  }
  if (in.token[0] != '\0')
    printf("Syntax error. \"%s\"\n", in.token);
  exit(0);

  return 0;
}
