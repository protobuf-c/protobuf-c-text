#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <google/protobuf-c/protobuf-c.h>
#include "lexer.h"
#include "parser.h"

static char *
esc_str(char *src, int len)
{
  int i, escapes = 0, dst_len = 0;
  unsigned char *dst;

  for (i = 0; i < len; i++) {
    if (!isprint(src[i])) {
      escapes++;
    }
  }
  dst = malloc((escapes * 4) + ((len - escapes) * 2) + 1);
  if (!dst) {
    return NULL;
  }

  for (i = 0; i < len; i++) {
    switch (src[i]) {
      /* Special cases. */
      case '\'':
        dst[dst_len++] = '\\';
        dst[dst_len++] = '\'';
        break;
      case '\"':
        dst[dst_len++] = '\\';
        dst[dst_len++] = '\"';
        break;
      case '\\':
        dst[dst_len++] = '\\';
        dst[dst_len++] = '\\';
        break;
      case '\n':
        dst[dst_len++] = '\\';
        dst[dst_len++] = 'n';
        break;
      case '\r':
        dst[dst_len++] = '\\';
        dst[dst_len++] = 'r';
        break;
      case '\t':
        dst[dst_len++] = '\\';
        dst[dst_len++] = 't';
        break;

      /* Escape with octal if !isprint. */
      default:
        if (!isprint(src[i])) {
          dst_len += sprintf(dst + dst_len, "\\%03o", src[i]);
        } else {
          dst[dst_len++] = src[i];
        }
        break;
    }
  }
  dst[dst_len] = '\0';

  return dst;
}

static int
text_format_parse(ProtobufCMessage *m, Scanner *s)
{
  int token_type;
  Token *t;
  void *p;

  p = ParseAlloc(malloc);
  while (token_type = scan(s, &t)) {
    switch (token_type) {
      case NUMBER:
        printf("NUMBER: %s\n", t->number);
        break;
      case BOOLEAN:
        printf("BOOLEAN: %s\n", t->boolean? "true": "false");
        break;
      case BAREWORD:
        printf("BAREWORD: %s\n", t->bareword);
        break;
      case QUOTED:
        printf("QUOTED: \"%s\"\n",
            esc_str(t->qs->data, (int)t->qs->len)
            );
        break;
      default:
        printf("%d: %.*s\n", token_type,
            (int)(s->cursor - s->token), s->token);
        break;
    }
    Parse(p, token_type, t);
  }
  Parse(p, 0, NULL);
  if (s->token[0] != '\0')
    printf("Syntax error. \"%s\"\n", s->token);
  ParseFree(p, free);

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
