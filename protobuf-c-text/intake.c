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
  int t;

  while (t = scan(s)) {
    switch (t) {
      case NUMBER:
        printf("NUMBER: %s\n", s->number);
        break;
      case BOOLEAN:
        printf("BOOLEAN: %s\n", s->boolean? "true": "false");
        break;
      case BAREWORD:
        printf("BAREWORD: %s\n", s->bareword);
        break;
      case QUOTED:
        printf("QUOTED: \"%s\"\n",
            esc_str(s->qs->data, (int)s->qs->len)
            );
        break;
      default:
        printf("%d: %.*s\n", t, (int)(s->cursor - s->token), s->token);
        break;
    }
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
