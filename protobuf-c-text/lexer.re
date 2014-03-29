/*
 * vim: set filetype=c:
 *
 * Compile with: re2c -s -o protobuf-c-text/lexer.re.c protobuf-c-text/lexer.re
 */


#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <google/protobuf-c/protobuf-c.h>
#include "protobuf-c-text.h"

typedef enum {
  TOK_EOF,
  TOK_BAREWORD,
  TOK_OBRACE,
  TOK_CBRACE,
  TOK_COLON,
  TOK_QUOTED,
  TOK_NUMBER,
  TOK_BOOLEAN
} TokenId;

typedef struct _Token {
  TokenId id;
  union {
    char *number;
    char *bareword;
    ProtobufCBinaryData *qs;
    bool boolean;
    char symbol;
  };
} Token;

typedef struct _Scanner {
  unsigned char *cursor;
  unsigned char *buffer;
  unsigned char *limit;
  unsigned char *token;
  FILE *f;
} Scanner;

static ProtobufCBinaryData *
unesc_str(unsigned char *src, int len)
{
  ProtobufCBinaryData *dst_pbbd;
  unsigned char *dst;
  int i = 0, dst_len = 0;
  unsigned char oct[4];

  dst_pbbd = malloc(sizeof(ProtobufCBinaryData));
  dst = malloc(len + 1);
  if (!dst_pbbd || !dst) {
    goto unesc_str_error;
  }
  oct[3] = '\0';

  while (i < len) {
    if (src[i] != '\\') {
      dst[dst_len++] = src[i++];
    } else {
      i++;
      if (i == len) {
        /* Fell off the end of the string after \. */
        goto unesc_str_error;
      }
      switch (src[i]) {
        case '0':
          if (i + 2 < len
              && (src[i+1] >= '0' && src[i+1] <= '7')
              && (src[i+2] >= '0' && src[i+2] <= '7')) {
            memcpy(oct, src + i, 3);
            dst[dst_len++] = (unsigned char)strtoul(oct, NULL, 8);
            i += 2;  /* Gets incremented again down below. */
          } else {
            /* Decoding a \0 failed or was cut off.. */
            goto unesc_str_error;
          }
          break;
        case '\'':
          dst[dst_len++] = '\'';
          break;
        case '\"':
          dst[dst_len++] = '\"';
          break;
        case '\\':
          dst[dst_len++] = '\\';
          break;
        case 'n':
          dst[dst_len++] = '\n';
          break;
        case 'r':
          dst[dst_len++] = '\r';
          break;
        case 't':
          dst[dst_len++] = '\t';
          break;
        default:
          goto unesc_str_error;
          break;
      }
      i++;
    }
  }

  dst_pbbd->data = dst;
  dst_pbbd->len = dst_len;
  return dst_pbbd;

unesc_str_error:
  free(dst);
  free(dst_pbbd);
  return NULL;
}

#define CHUNK 4096

static int
fill(Scanner *s)
{
  char *buf;
  int len, oldlen, nmemb;

  if (s->token > s->limit) {
    /* this shouldn't happen */
    return 0;
  }
  if (s->f && !feof(s->f)) {
    oldlen = s->limit - s->token;
    buf = malloc(CHUNK + oldlen);
    memcpy(buf, s->token, oldlen);
    nmemb = fread(buf + oldlen, 1, CHUNK, s->f);
    if (nmemb != CHUNK) {
      /* Short read.  eof.  Append nul. */
      len = oldlen + nmemb;
      buf[len] = '\0';
    }
    /* Reset the world to use buf. */
    s->cursor = &buf[s->cursor - s->token];
    s->limit = buf + len;
    s->token = buf;
    free(s->buffer);
    s->buffer = buf;
  }

  return s->limit >= s->cursor;
}

#define RETURN(tt) { t.id = tt; return t; }
#define YYFILL(n) { if (!fill(s)) RETURN(TOK_EOF); }

static Token
scan(Scanner *s)
{
  Token t;

token_start:
  s->token = s->cursor;

  /*!re2c
  re2c:define:YYCTYPE   = "unsigned char";
  re2c:define:YYCURSOR  = s->cursor;
  re2c:define:YYLIMIT   = s->limit;
  re2c:define:YYMARKER  = s->buffer;

  I = [-]? [0-9]+;
  F = [-]? [0-9]* "." [0-9]+;
  BW = [a-zA-Z0-9_]+;
  EQ = [\\] ["];
  NL = "\n";
  QS = ["] (EQ|[^"]|NL)* ["];
  WS = [ \t\n];

  I | F           {
                    t.number = malloc((s->cursor - s->token) + 1);
                    memcpy(t.number, s->token, s->cursor - s->token);
                    t.number[s->cursor - s->token] = '\0';
                    RETURN(TOK_NUMBER);
                  }
  "true"          { t.boolean=true; RETURN(TOK_BOOLEAN); }
  "false"         { t.boolean=false; RETURN(TOK_BOOLEAN); }
  BW              {
                    t.bareword = malloc((s->cursor - s->token) + 1);
                    memcpy(t.bareword, s->token, s->cursor - s->token);
                    t.bareword[s->cursor - s->token] = '\0';
                    RETURN(TOK_BAREWORD);
                  }
  QS              {
                    t.qs = unesc_str(s->token + 1,
                                      s->cursor - s->token - 2);
                    RETURN(TOK_QUOTED);
                  }
  "{"             { t.symbol = '{'; RETURN(TOK_OBRACE); }
  "}"             { t.symbol = '}'; RETURN(TOK_CBRACE); }
  ":"             { t.symbol = ':'; RETURN(TOK_COLON); }
  WS              { goto token_start; }
  "\000"          { RETURN(TOK_EOF); }
  */
}

static void
scanner_init_file(Scanner *s, FILE *f)
{
  memset(s, 0, sizeof(Scanner));
  s->f = f;
}

static void
scanner_init_string(Scanner *s, char *buf)
{
  memset(s, 0, sizeof(Scanner));
  s->buffer = buf;
  s->cursor = buf;
  s->limit = &buf[strlen(buf)];
}

typedef enum {
  STATE_OPEN,
  STATE_ASSIGNMENT,
  STATE_VALUE,
  STATE_DONE
} StateId;

typedef struct {
  char *element;
  int current_msg;
  int max_msgs;
  ProtobufCMessage **msgs;
  char *error;
} State;

/*
 * Helper function to handle errors.
 */
static StateId
state_error(State *state, Token *t, char *error_string)
{
  state->error = error_string;
  return STATE_DONE;
}

/*
 * Expect an element name (bareword) or a closing brace.
 */
static StateId
state_open(State *state, Token *t)
{
  switch (t->id) {
    case TOK_BAREWORD:
      printf("Assign something to %s\n", t->bareword);
      return STATE_ASSIGNMENT;
      break;
    case TOK_CBRACE:
      printf("Pop the message stack.\n");
      return STATE_OPEN;
      break;
    case TOK_EOF:
      printf("Parsing complete.\n");
      return STATE_DONE;
      break;
    default:
      state_error(state, t, "Expected element name or closing brace.");
      return STATE_DONE;
      break;
  }
}

/*
 * Expect a colon or opening brace.
 */
static StateId
state_assignment(State *state, Token *t)
{
  switch (t->id) {
    case TOK_COLON:
      printf("Assign scalar.\n");
      return STATE_VALUE;
      break;
    case TOK_OBRACE:
      printf("Push the message stack.\n");
      return STATE_OPEN;
      break;
    default:
      state_error(state, t, "Expected colon or opening brace.");
      return STATE_DONE;
      break;
  }
}

/*
 * Expect a quoted string, enum (bareword) or boolean.
 */
static StateId
state_value(State *state, Token *t)
{
  switch (t->id) {
    case TOK_BAREWORD:
      printf("Assign enum.\n");
      return STATE_OPEN;
      break;
    case TOK_BOOLEAN:
      printf("Assign boolean.\n");
      return STATE_OPEN;
      break;
    case TOK_QUOTED:
      printf("Assign boolean.\n");
      return STATE_OPEN;
      break;
    case TOK_NUMBER:
      printf("Assign number.\n");
      return STATE_OPEN;
      break;
    default:
      state_error(state, t, "Expected value.");
      return STATE_DONE;
      break;
  }
}

static StateId(* states[])(State *, Token *) = {
  [STATE_OPEN] = state_open,
  [STATE_ASSIGNMENT] = state_assignment,
  [STATE_VALUE] = state_value
};

static int
text_format_parse(ProtobufCMessage *msg, Scanner *scanner)
{
  Token token;
  State state;
  StateId state_id;

  state_id = STATE_OPEN;
  memset(&state, 0, sizeof(State));
  state.msgs = malloc(10 * sizeof(ProtobufCMessage *));
  state.max_msgs = 10;
  state.msgs[0] = msg;

  while (state_id != STATE_DONE) {
    token = scan(scanner);
    state_id = states[state_id](&state, &token);
  }

  if (state.error) {
    printf("ERROR: %s\n", state.error);
  }

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
