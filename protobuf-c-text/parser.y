%include {
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

}

%token_type {Token *}
%token_destructor {
  if ($$) {
    switch ($$->token_type) {
      case NUMBER:
        free($$->number);
        break;
      case BAREWORD:
        free($$->bareword);
        break;
      case QUOTED:
        free($$->qs->data);
        free($$->qs);
        break;
    }
    free($$);
  }
}

%syntax_error {
  printf("Syntax error!\n");
}

state ::= messages.
messages ::= messages message.
messages ::= .
message ::= BAREWORD(A) OBRACE statements CBRACE. {
        printf("Got a message for (%s).\n", A->bareword);
        }

statement ::= BAREWORD(A) COLON BAREWORD(B). {
           printf("Got a enum statement: %s <- %s.\n",
                  A->bareword, B->bareword);
           }
statement ::= BAREWORD(A) COLON QUOTED(B). {
           printf("Got a string statement: %s <- \"%s\".\n",
                  A->bareword, esc_str(B->qs->data, B->qs->len));
           }
statement ::= BAREWORD(A) COLON NUMBER(B). {
           printf("Got a number statement.\n",
                  A->bareword, B->number);
           }
statement ::= BAREWORD(A) COLON BOOLEAN(B). {
           printf("Got a boolean statement.\n",
                  A->bareword, B->boolean? "true": "false");
           }
statement ::= message. {
           printf("Got a message statement.\n");
           }
statements ::= statements statement.
statements ::= .
