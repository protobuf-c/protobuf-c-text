#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <google/protobuf-c/protobuf-c.h>
#include "lexer.h"
#include "parser.h"

static int
text_format_parse(ProtobufCMessage *m, Scanner *s)
{
  int t;

  while (t = scan(s)) {
    printf("%d: %.*s\n", t, (int)(s->cursor - s->token), s->token);
  }
  if (s->token[0] != '\0')
    printf("Syntax error. \"%s\"\n", s->token);
  exit(0);

  return 0;
}

int
text_format_from_file(ProtobufCMessage *m, FILE *msg_file)
{
  Scanner scanner;

  scanner_init_file(&scanner, msg_file);
  return text_format_parse(m, &scanner);
}

int
text_format_from_string(ProtobufCMessage *m, char *msg)
{
  Scanner scanner;

  scanner_init_string(&scanner, msg);
  return text_format_parse(m, &scanner);
}
