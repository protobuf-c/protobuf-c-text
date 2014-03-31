/*
 * vim: set filetype=c:
 *
 * Compile with: re2c -s -o protobuf-c-text/parse.c protobuf-c-text/parse.re
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <google/protobuf-c/protobuf-c.h>
#include "protobuf-c-text.h"
#include "protobuf-c-util.h"
#include "config.h"

static void *
local_realloc(void *ptr,
    size_t old_size,
    size_t size,
    ProtobufCAllocator *allocator)
{
  void *tmp;

  tmp = allocator->alloc(allocator->allocator_data, size);
  if (!tmp) {
    return NULL;
  }
  if (old_size < size) {
    /* Extending. */
    memcpy(tmp, ptr, old_size);
  } else {
    /* Truncating. */
    memcpy(tmp, ptr, size);
  }

  allocator->free(allocator->allocator_data, ptr);
  return tmp;
}

typedef enum {
  TOK_EOF,
  TOK_BAREWORD,
  TOK_OBRACE,
  TOK_CBRACE,
  TOK_COLON,
  TOK_QUOTED,
  TOK_NUMBER,
  TOK_BOOLEAN,
  TOK_MALLOC_ERR
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

static char *
token2txt(Token *t)
{
  switch (t->id) {
    case TOK_EOF:
      return "[EOF]"; break;
    case TOK_BAREWORD:
      return t->bareword; break;
    case TOK_OBRACE:
      return "{"; break;
    case TOK_CBRACE:
      return "}"; break;
    case TOK_COLON:
      return ":"; break;
    case TOK_QUOTED:
      return "[string]"; break;
    case TOK_NUMBER:
      return t->number; break;
    case TOK_BOOLEAN:
      return t->boolean? "true": "false"; break;
    default:
      return "[UNKNOWN]"; break;
  }
}

static void
token_free(Token *t, ProtobufCAllocator *allocator)
{
  switch (t->id) {
    case TOK_BAREWORD:
      allocator->free(allocator->allocator_data, t->bareword);
      break;
    case TOK_QUOTED:
      allocator->free(allocator->allocator_data, t->qs->data);
      break;
    case TOK_NUMBER:
      allocator->free(allocator->allocator_data, t->number);
      break;
    default:
      break;
  }
}

typedef struct _Scanner {
  unsigned char *cursor;
  unsigned char *marker;
  unsigned char *buffer;
  unsigned char *limit;
  unsigned char *token;
  FILE *f;
  int line;
} Scanner;

static void
scanner_init_file(Scanner *scanner, FILE *f)
{
  memset(scanner, 0, sizeof(Scanner));
  scanner->f = f;
  scanner->line = 1;
}

static void
scanner_init_string(Scanner *scanner, char *buf)
{
  memset(scanner, 0, sizeof(Scanner));
  scanner->buffer = buf;
  scanner->marker = buf;
  scanner->cursor = buf;
  scanner->limit = &buf[strlen(buf)];
  scanner->line = 1;
}

static void
scanner_free(Scanner *scanner, ProtobufCAllocator *allocator)
{
  if (scanner->f && scanner->buffer)
    allocator->free(allocator->allocator_data, scanner->buffer);
  scanner->buffer = NULL;
}

static ProtobufCBinaryData *
unesc_str(unsigned char *src, int len, ProtobufCAllocator *allocator)
{
  ProtobufCBinaryData *dst_pbbd;
  unsigned char *dst;
  int i = 0, dst_len = 0;
  unsigned char oct[4];

  dst_pbbd = allocator->alloc(allocator->allocator_data,
      sizeof(ProtobufCBinaryData));
  dst = allocator->alloc(allocator->allocator_data, len + 1);
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
  allocator->free(allocator->allocator_data, dst);
  allocator->free(allocator->allocator_data, dst_pbbd);
  return NULL;
}

#define CHUNK 4096

static int
fill(Scanner *scanner, ProtobufCAllocator *allocator)
{
  char *buf;
  int len, oldlen, nmemb;

  if (scanner->token > scanner->limit) {
    /* this shouldn't happen */
    return 0;
  }
  if (scanner->f && !feof(scanner->f)) {
    oldlen = scanner->limit - scanner->token;
    len = CHUNK + oldlen;
    buf = allocator->alloc(allocator->allocator_data, len);
    if (!buf) {
      return -1;
    }
    memcpy(buf, scanner->token, oldlen);
    nmemb = fread(buf + oldlen, 1, CHUNK, scanner->f);
    if (nmemb != CHUNK) {
      /* Short read.  eof.  Append nul. */
      len = oldlen + nmemb;
      buf[len] = '\0';
    }
    /* Reset the world to use buf. */
    scanner->cursor = &buf[scanner->cursor - scanner->token];
    scanner->limit = buf + len;
    scanner->token = buf;
    allocator->free(allocator->allocator_data, scanner->buffer);
    scanner->buffer = buf;
    scanner->marker = buf;
  }

  return scanner->limit >= scanner->cursor? 1: 0;
}

#define RETURN(tt) { t.id = tt; return t; }
#define YYFILL(n) { fill_result = fill(scanner, allocator); \
                    if (fill_result <=0) \
                      RETURN((fill_result == -1? TOK_MALLOC_ERR: TOK_EOF)); }

static Token
scan(Scanner *scanner, ProtobufCAllocator *allocator)
{
  Token t;
  int fill_result;

token_start:
  scanner->token = scanner->cursor;

  /*!re2c
  re2c:define:YYCTYPE   = "unsigned char";
  re2c:define:YYCURSOR  = scanner->cursor;
  re2c:define:YYLIMIT   = scanner->limit;
  re2c:define:YYMARKER  = scanner->marker;

  I = [-]? [0-9]+;
  F = [-]? [0-9]* "." [0-9]+;
  BW = [a-zA-Z0-9_]+;
  EQ = [\\] ["];
  NL = "\n";
  QS = ["] (EQ|[^"]|NL)* ["];
  WS = [ \t];

  I | F       {
                t.number = allocator->alloc(allocator->allocator_data,
                       (scanner->cursor - scanner->token) + 1);
                if (!t.number) {
                  RETURN(TOK_MALLOC_ERR);
                }
                memcpy(t.number, scanner->token,
                       scanner->cursor - scanner->token);
                t.number[scanner->cursor - scanner->token] = '\0';
                RETURN(TOK_NUMBER);
              }
  "true"      { t.boolean=true; RETURN(TOK_BOOLEAN); }
  "false"     { t.boolean=false; RETURN(TOK_BOOLEAN); }
  BW          {
                t.bareword = allocator->alloc(allocator->allocator_data,
                       (scanner->cursor - scanner->token) + 1);
                if (!t.bareword) {
                  RETURN(TOK_MALLOC_ERR);
                }
                memcpy(t.bareword, scanner->token,
                       scanner->cursor - scanner->token);
                t.bareword[scanner->cursor - scanner->token] = '\0';
                RETURN(TOK_BAREWORD);
              }
  QS          {
                t.qs = unesc_str(scanner->token + 1,
                                 scanner->cursor - scanner->token - 2,
                                 allocator);
                if (!t.qs) {
                  RETURN(TOK_MALLOC_ERR);
                }
                RETURN(TOK_QUOTED);
              }
  "{"         { t.symbol = '{'; RETURN(TOK_OBRACE); }
  "}"         { t.symbol = '}'; RETURN(TOK_CBRACE); }
  ":"         { t.symbol = ':'; RETURN(TOK_COLON); }
  WS          { goto token_start; }
  NL          { scanner->line++; goto token_start; }
  "\000"      { RETURN(TOK_EOF); }
  */
}

typedef enum {
  STATE_OPEN,
  STATE_ASSIGNMENT,
  STATE_VALUE,
  STATE_DONE
} StateId;

#define STATE_ERROR_STR_MAX 160
typedef struct {
  Scanner *scanner;
  const ProtobufCFieldDescriptor *field;
  int current_msg;
  int max_msg;
  ProtobufCMessage **msgs;
  ProtobufCAllocator *allocator;
  int error;
  char *error_str;
} State;

static int
state_init(State *state,
    Scanner *scanner,
    const ProtobufCMessageDescriptor *descriptor,
    ProtobufCAllocator *allocator)
{
  ProtobufCMessage *msg;

  memset(state, 0, sizeof(State));
  state->allocator = allocator;
  state->scanner = scanner;
  state->error_str = state->allocator->alloc(
      state->allocator->allocator_data, STATE_ERROR_STR_MAX);
  state->msgs = state->allocator->alloc(state->allocator->allocator_data,
      10 * sizeof(ProtobufCMessage *));
  state->max_msg = 10;
  msg = state->allocator->alloc(state->allocator->allocator_data,
      descriptor->sizeof_message);
  if (!state->msgs || !msg || !state->error_str) {
    state->allocator->free(state->allocator->allocator_data,
        state->error_str);
    state->allocator->free(state->allocator->allocator_data, state->msgs);
    state->allocator->free(state->allocator->allocator_data, msg);
    return 0;
  }
  descriptor->message_init(msg);
  state->msgs[0] = msg;

  return 1;
}

static void
state_free(State *state)
{
  state->allocator->free(state->allocator->allocator_data, state->msgs);
}

/*
 * Helper function to handle errors.
 */
static StateId state_error(State *state, Token *t, char *error_fmt, ...)
  __attribute__((format(printf, 3, 4)));
static StateId
state_error(State *state, Token *t, char *error_fmt, ...)
{
  va_list args;
  int error_idx;

  /* 10 solid lines of errors is more than enough. */
  state->error = 1;
  va_start(args, error_fmt);
  error_idx = vsnprintf(state->error_str, STATE_ERROR_STR_MAX,
      error_fmt, args);
  va_end(args);

  if (error_idx < STATE_ERROR_STR_MAX) {
    snprintf(state->error_str + error_idx,
        STATE_ERROR_STR_MAX - error_idx,
        "\nError found on line %d.\n", state->scanner->line);
  }

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
      state->field = protobuf_c_message_descriptor_get_field_by_name(
          state->msgs[state->current_msg]->descriptor, t->bareword);
      if (state->field) {
        if (state->field->label != PROTOBUF_C_LABEL_REQUIRED
            && state->field->label != PROTOBUF_C_LABEL_OPTIONAL
            && state->field->label != PROTOBUF_C_LABEL_REPEATED) {
          return state_error(state, t,
              "Internal error: unknown label type %d for '%s'.",
              state->field->label, state->field->name);
        }
        return STATE_ASSIGNMENT;
      } else {
        return state_error(state, t, "Can't find field '%s' in message '%s'.",
                           t->bareword,
                           state->msgs[state->current_msg]->descriptor->name);
      }
      break;
    case TOK_CBRACE:
      if (state->current_msg > 0) {
        /* TODO: Check all required fields have been set. */
        state->current_msg--;
      } else {
        return state_error(state, t, "Extra closing brace found.");
      }
      return STATE_OPEN;
      break;
    case TOK_EOF:
      if (state->current_msg > 0) {
        return state_error(state, t, "Missing '%d' closing braces.",
            state->current_msg);
      }
      return STATE_DONE;
      break;
    default:
      return state_error(state, t,
                         "Expected element name or '}'; found '%s' instead.",
                         token2txt(t));
      break;
  }
}

/*
 * Expect a colon or opening brace.
 */
static StateId
state_assignment(State *state, Token *t)
{
  ProtobufCMessage *msg;

  msg = state->msgs[state->current_msg];
  switch (t->id) {
    case TOK_COLON:
      if (state->field->type == PROTOBUF_C_TYPE_MESSAGE) {
        return state_error(state, t,
            "Expected a '{', got a ':' - '%s' is a message type field.",
            state->field->name);
      }
      return STATE_VALUE;
      break;
    case TOK_OBRACE:
      if (state->field->type == PROTOBUF_C_TYPE_MESSAGE) {
        if (state->field->label == PROTOBUF_C_LABEL_OPTIONAL
            || state->field->label == PROTOBUF_C_LABEL_REQUIRED) {
          /* Do optional member accounting. */
          if (STRUCT_MEMBER(protobuf_c_boolean, msg,
                state->field->offset)) {
            return state_error(state, t,
                "The '%s' message has already been assigned.",
                state->field->name);
          }
        }
        if (state->field->label == PROTOBUF_C_LABEL_REPEATED) {
          ProtobufCMessage **tmp;
          size_t n_members;

          /* Create new message and assign it to the message stack. */
          state->current_msg++;
          if (state->current_msg == state->max_msg) {
            /* TODO: Dynamically increase msgs. */
            return state_error(state, t,
                "'%s' is too many messages deep.", state->field->name);
          }
          STRUCT_MEMBER(size_t, msg, state->field->quantifier_offset) += 1;
          n_members = STRUCT_MEMBER(size_t, msg,
                                    state->field->quantifier_offset);
          tmp = local_realloc(
              STRUCT_MEMBER(ProtobufCMessage *, msg, state->field->offset),
              (n_members - 1) * sizeof(ProtobufCMessage *),
              n_members * sizeof(ProtobufCMessage *),
              state->allocator);
          if (!tmp) {
            return state_error(state, t, "Malloc failure.");
          }
          STRUCT_MEMBER(ProtobufCMessage **, msg, state->field->offset)
            = tmp;
          tmp[n_members - 1] = state->allocator->alloc(
              state->allocator->allocator_data,
              ((ProtobufCMessageDescriptor *)
                state->field->descriptor)->sizeof_message);
          if (!tmp[n_members - 1]) {
            return state_error(state, t, "Malloc failure.");
          }
          state->msgs[state->current_msg] = tmp[n_members - 1];
          ((ProtobufCMessageDescriptor *)state->field->descriptor)
            ->message_init(tmp[n_members - 1]);
          return STATE_OPEN;
        } else {
          /* Create new message and assign it to the message stack. */
          state->current_msg++;
          if (state->current_msg == state->max_msg) {
            /* TODO: Dynamically increase msgs. */
            return state_error(state, t,
                "'%s' is too many messages deep.", state->field->name);
          }
          state->msgs[state->current_msg]
            = state->allocator->alloc(state->allocator->allocator_data,
                ((ProtobufCMessageDescriptor *)
                 state->field->descriptor)->sizeof_message);
          STRUCT_MEMBER(ProtobufCMessage *, msg, state->field->offset)
            = state->msgs[state->current_msg];
          ((ProtobufCMessageDescriptor *)state->field->descriptor)
            ->message_init(state->msgs[state->current_msg]);
          return STATE_OPEN;
        }

      } else {
        return state_error(state, t,
            "'%s' is not a message field.", state->field->name);
      }
      break;
    default:
      return state_error(state, t,
                         "Expected ':' or '{'; found '%s' instead.",
                         token2txt(t));
      break;
  }
}

/*
 * Expect a quoted string, enum (bareword) or boolean.
 */
static StateId
state_value(State *state, Token *t)
{
  ProtobufCMessage *msg;
  size_t n_members;
  char *end;

  msg = state->msgs[state->current_msg];
  if (state->field->type != PROTOBUF_C_TYPE_STRING) {
    if (state->field->label == PROTOBUF_C_LABEL_OPTIONAL) {
      /* Do optional member accounting. */
      if (STRUCT_MEMBER(protobuf_c_boolean, msg,
            state->field->quantifier_offset)) {
        return state_error(state, t,
            "'%s' has already been assigned.", state->field->name);
      }
      STRUCT_MEMBER(protobuf_c_boolean, msg,
            state->field->quantifier_offset) = 1;
    }
  }
  switch (t->id) {
    case TOK_BAREWORD:
      if (state->field->type == PROTOBUF_C_TYPE_ENUM) {
        ProtobufCEnumDescriptor *enumd;
        const ProtobufCEnumValue *enumv;

        enumd = (ProtobufCEnumDescriptor *)state->field->descriptor;
        enumv = protobuf_c_enum_descriptor_get_value_by_name(enumd,
            t->bareword);
        if (enumv) {
          if (state->field->label == PROTOBUF_C_LABEL_REPEATED) {
            int *tmp;

            STRUCT_MEMBER(size_t, msg, state->field->quantifier_offset) += 1;
            n_members = STRUCT_MEMBER(size_t, msg,
                state->field->quantifier_offset);
            tmp = local_realloc(
                STRUCT_MEMBER(int *, msg, state->field->offset),
                (n_members - 1) * sizeof(int),
                n_members * sizeof(int), state->allocator);
            if (!tmp) {
              return state_error(state, t, "Malloc failure.");
            }
            STRUCT_MEMBER(int *, msg, state->field->offset) = tmp;
            tmp[n_members - 1] = enumv->value;
            return STATE_OPEN;
          } else {
            STRUCT_MEMBER(int, msg, state->field->offset) = enumv->value;
            return STATE_OPEN;
          }
        } else {
          return state_error(state, t,
              "Invalid enum '%s' for field '%s'.",
              t->bareword, state->field->name);
        }
      }
      return state_error(state, t,
          "'%s' is not an enum field.", state->field->name);
      break;

    case TOK_BOOLEAN:
      if (state->field->type == PROTOBUF_C_TYPE_BOOL) {
        if (state->field->label == PROTOBUF_C_LABEL_REPEATED) {
          protobuf_c_boolean *tmp;

          STRUCT_MEMBER(size_t, msg, state->field->quantifier_offset) += 1;
          n_members = STRUCT_MEMBER(size_t, msg,
                                    state->field->quantifier_offset);
          tmp = local_realloc(
              STRUCT_MEMBER(protobuf_c_boolean *, msg, state->field->offset),
              (n_members - 1) * sizeof(protobuf_c_boolean),
              n_members * sizeof(protobuf_c_boolean), state->allocator);
          if (!tmp) {
            return state_error(state, t, "Malloc failure.");
          }
          STRUCT_MEMBER(protobuf_c_boolean *, msg, state->field->offset) = tmp;
          tmp[n_members - 1] = t->boolean;
          return STATE_OPEN;
        } else {
          STRUCT_MEMBER(protobuf_c_boolean, msg, state->field->offset)
            = t->boolean;
          return STATE_OPEN;
        }

      }
      return state_error(state, t,
          "'%s' is not a boolean field.", state->field->name);
      break;

    case TOK_QUOTED:
      if (state->field->type == PROTOBUF_C_TYPE_BYTES) {
        ProtobufCBinaryData *pbbd;

        if (state->field->label == PROTOBUF_C_LABEL_REPEATED) {
          STRUCT_MEMBER(size_t, msg, state->field->quantifier_offset) += 1;
          n_members = STRUCT_MEMBER(size_t, msg,
                                    state->field->quantifier_offset);
          pbbd = local_realloc(
              STRUCT_MEMBER(ProtobufCBinaryData *, msg, state->field->offset),
              (n_members - 1) * sizeof(ProtobufCBinaryData),
              n_members * sizeof(ProtobufCBinaryData), state->allocator);
          if (!pbbd) {
            return state_error(state, t, "Malloc failure.");
          }
          STRUCT_MEMBER(ProtobufCBinaryData *, msg, state->field->offset)
            = pbbd;
          pbbd[n_members - 1].data = state->allocator->alloc(
              state->allocator->allocator_data, t->qs->len);
          memcpy(pbbd[n_members - 1].data, t->qs->data, t->qs->len);
          pbbd[n_members - 1].len = t->qs->len;
          return STATE_OPEN;
        } else {
          pbbd = STRUCT_MEMBER_PTR(ProtobufCBinaryData, msg,
              state->field->offset);
          pbbd->data = state->allocator->alloc(
              state->allocator->allocator_data, t->qs->len);
          if (!pbbd->data) {
            return state_error(state, t, "Malloc failure.");
          }
          memcpy(pbbd->data, t->qs->data, t->qs->len);
          pbbd->len = t->qs->len;
          return STATE_OPEN;
        }

      } else if (state->field->type == PROTOBUF_C_TYPE_STRING) {
        if (state->field->label == PROTOBUF_C_LABEL_OPTIONAL) {
          /* Do optional member accounting. */
          if (STRUCT_MEMBER(unsigned char *, msg, state->field->offset)
              && (STRUCT_MEMBER(unsigned char *, msg, state->field->offset)
                != state->field->default_value)) {
            return state_error(state, t,
                "'%s' has already been assigned.", state->field->name);
          }
        }
        if (state->field->label == PROTOBUF_C_LABEL_REPEATED) {
          unsigned char **s;

          STRUCT_MEMBER(size_t, msg, state->field->quantifier_offset) += 1;
          n_members = STRUCT_MEMBER(size_t, msg,
                                    state->field->quantifier_offset);
          s = local_realloc(
              STRUCT_MEMBER(unsigned char **, msg, state->field->offset),
              (n_members - 1) * sizeof(unsigned char *),
              n_members * sizeof(unsigned char *), state->allocator);
          if (!s) {
            return state_error(state, t, "Malloc failure.");
          }
          STRUCT_MEMBER(unsigned char **, msg, state->field->offset) = s;
          s[n_members - 1] = state->allocator->alloc(
              state->allocator->allocator_data, t->qs->len + 1);
          if (!s[n_members - 1]) {
            return state_error(state, t, "Malloc failure.");
          }
          memcpy(s[n_members - 1], t->qs->data, t->qs->len);
          s[n_members - 1][t->qs->len] = '\0';
          return STATE_OPEN;
        } else {
          unsigned char *s;

          s = state->allocator->alloc(state->allocator->allocator_data,
              t->qs->len + 1);
          if (!s) {
            return state_error(state, t, "Malloc failure.");
          }
          memcpy(s, t->qs->data, t->qs->len);
          s[t->qs->len] = '\0';
          if (strlen(s) != t->qs->len) {
            /* TODO: Error if there's an embedded NUL in a string? */
          }
          STRUCT_MEMBER(unsigned char *, msg, state->field->offset) = s;
          return STATE_OPEN;
        }

      }
      return state_error(state, t,
          "'%s' is not a boolean field.", state->field->name);
      break;

    case TOK_NUMBER:
      switch (state->field->type) {
        case PROTOBUF_C_TYPE_INT32:
        case PROTOBUF_C_TYPE_UINT32:
        case PROTOBUF_C_TYPE_FIXED32:
          if (state->field->label == PROTOBUF_C_LABEL_REPEATED) {
            uint32_t *vals;
            uint64_t val;

            STRUCT_MEMBER(size_t, msg, state->field->quantifier_offset) += 1;
            n_members = STRUCT_MEMBER(size_t, msg,
                                      state->field->quantifier_offset);
            vals = local_realloc(
                STRUCT_MEMBER(uint32_t *, msg, state->field->offset),
                (n_members - 1) * sizeof(uint32_t),
                n_members * sizeof(uint32_t), state->allocator);
            if (!vals) {
              return state_error(state, t, "Malloc failure.");
            }
            STRUCT_MEMBER(uint32_t *, msg, state->field->offset) = vals;
            val = strtoul(t->number, &end, 10);
            if (*end != '\0' || val > UINT32_MAX) {
              return state_error(state, t,
                  "Unable to convert '%s' for field '%s'.",
                  t->number, state->field->name);
            }
            vals[n_members - 1] = (uint32_t)val;
            return STATE_OPEN;
          } else {
            uint64_t val;

            val = strtoul(t->number, &end, 10);
            if (*end != '\0' || val > UINT32_MAX) {
              return state_error(state, t,
                  "Unable to convert '%s' for field '%s'.",
                  t->number, state->field->name);
            }
            STRUCT_MEMBER(uint32_t, msg, state->field->offset) = (uint32_t)val;
            return STATE_OPEN;
          }
          break;

        case PROTOBUF_C_TYPE_SINT32:
        case PROTOBUF_C_TYPE_SFIXED32:
          if (state->field->label == PROTOBUF_C_LABEL_REPEATED) {
            int32_t *vals;
            int64_t val;

            STRUCT_MEMBER(size_t, msg, state->field->quantifier_offset) += 1;
            n_members = STRUCT_MEMBER(size_t, msg,
                                      state->field->quantifier_offset);
            vals = local_realloc(
                STRUCT_MEMBER(int32_t *, msg, state->field->offset),
                (n_members - 1) * sizeof(int32_t),
                n_members * sizeof(int32_t), state->allocator);
            if (!vals) {
              return state_error(state, t, "Malloc failure.");
            }
            STRUCT_MEMBER(int32_t *, msg, state->field->offset) = vals;
            val = strtol(t->number, &end, 10);
            if (*end != '\0' || val < INT32_MIN || val > INT32_MAX) {
              return state_error(state, t,
                  "Unable to convert '%s' for field '%s'.",
                  t->number, state->field->name);
            }
            vals[n_members - 1] = (uint32_t)val;
            return STATE_OPEN;
          } else {
            int32_t val;

            val = strtol(t->number, &end, 10);
            if (*end != '\0' || val < INT32_MIN || val > INT32_MAX) {
              return state_error(state, t,
                  "Unable to convert '%s' for field '%s'.",
                  t->number, state->field->name);
            }
            STRUCT_MEMBER(int32_t, msg, state->field->offset) = val;
            return STATE_OPEN;
          }
          break;

        case PROTOBUF_C_TYPE_INT64:
        case PROTOBUF_C_TYPE_UINT64:
        case PROTOBUF_C_TYPE_FIXED64:
          if (state->field->label == PROTOBUF_C_LABEL_REPEATED) {
            uint64_t *vals;

            STRUCT_MEMBER(size_t, msg, state->field->quantifier_offset) += 1;
            n_members = STRUCT_MEMBER(size_t, msg,
                                      state->field->quantifier_offset);
            vals = local_realloc(
                STRUCT_MEMBER(uint64_t *, msg, state->field->offset),
                (n_members - 1) * sizeof(uint64_t),
                n_members * sizeof(uint64_t), state->allocator);
            if (!vals) {
              return state_error(state, t, "Malloc failure.");
            }
            STRUCT_MEMBER(uint64_t *, msg, state->field->offset) = vals;
            vals[n_members - 1] = strtoull(t->number, &end, 10);
            if (*end != '\0') {
              return state_error(state, t,
                  "Unable to convert '%s' for field '%s'.",
                  t->number, state->field->name);
            }
            return STATE_OPEN;
          } else {
            uint64_t val;

            val = strtoull(t->number, &end, 10);
            if (*end != '\0') {
              return state_error(state, t,
                  "Unable to convert '%s' for field '%s'.",
                  t->number, state->field->name);
            }
            STRUCT_MEMBER(uint64_t, msg, state->field->offset) = val;
            return STATE_OPEN;
          }
          break;

        case PROTOBUF_C_TYPE_SINT64:
        case PROTOBUF_C_TYPE_SFIXED64:
          if (state->field->label == PROTOBUF_C_LABEL_REPEATED) {
            int64_t *vals;

            STRUCT_MEMBER(size_t, msg, state->field->quantifier_offset) += 1;
            n_members = STRUCT_MEMBER(size_t, msg,
                                      state->field->quantifier_offset);
            vals = local_realloc(
                STRUCT_MEMBER(int64_t *, msg, state->field->offset),
                (n_members - 1) * sizeof(int64_t),
                n_members * sizeof(int64_t), state->allocator);
            if (!vals) {
              return state_error(state, t, "Malloc failure.");
            }
            STRUCT_MEMBER(int64_t *, msg, state->field->offset) = vals;
            vals[n_members - 1] = strtol(t->number, &end, 10);
            if (*end != '\0') {
              return state_error(state, t,
                  "Unable to convert '%s' for field '%s'.",
                  t->number, state->field->name);
            }
            return STATE_OPEN;
          } else {
            int64_t val;

            val = strtoll(t->number, &end, 10);
            if (*end != '\0') {
              return state_error(state, t,
                  "Unable to convert '%s' for field '%s'.",
                  t->number, state->field->name);
            }
            STRUCT_MEMBER(int64_t, msg, state->field->offset) = val;
            return STATE_OPEN;
          }
          break;

        case PROTOBUF_C_TYPE_FLOAT:
          if (state->field->label == PROTOBUF_C_LABEL_REPEATED) {
            float *vals;

            STRUCT_MEMBER(size_t, msg, state->field->quantifier_offset) += 1;
            n_members = STRUCT_MEMBER(size_t, msg,
                                      state->field->quantifier_offset);
            vals = local_realloc(
                STRUCT_MEMBER(float *, msg, state->field->offset),
                (n_members - 1) * sizeof(float),
                n_members * sizeof(float), state->allocator);
            if (!vals) {
              return state_error(state, t, "Malloc failure.");
            }
            STRUCT_MEMBER(float *, msg, state->field->offset) = vals;
            vals[n_members - 1] = strtof(t->number, &end);
            if (*end != '\0') {
              return state_error(state, t,
                  "Unable to convert '%s' for field '%s'.",
                  t->number, state->field->name);
            }
            return STATE_OPEN;
          } else {
            float val;

            val = strtof(t->number, &end);
            if (*end != '\0') {
              return state_error(state, t,
                  "Unable to convert '%s' for field '%s'.",
                  t->number, state->field->name);
            }
            STRUCT_MEMBER(float, msg, state->field->offset) = val;
            return STATE_OPEN;
          }
          break;

        case PROTOBUF_C_TYPE_DOUBLE:
          if (state->field->label == PROTOBUF_C_LABEL_REPEATED) {
            double *vals;

            STRUCT_MEMBER(size_t, msg, state->field->quantifier_offset) += 1;
            n_members = STRUCT_MEMBER(size_t, msg,
                                      state->field->quantifier_offset);
            vals = local_realloc(
                STRUCT_MEMBER(double *, msg, state->field->offset),
                (n_members - 1) * sizeof(double),
                n_members * sizeof(double), state->allocator);
            if (!vals) {
              return state_error(state, t, "Malloc failure.");
            }
            STRUCT_MEMBER(double *, msg, state->field->offset) = vals;
            vals[n_members - 1] = strtod(t->number, &end);
            if (*end != '\0') {
              return state_error(state, t,
                  "Unable to convert '%s' for field '%s'.",
                  t->number, state->field->name);
            }
            return STATE_OPEN;
          } else {
            double val;

            val = strtod(t->number, &end);
            if (*end != '\0') {
              return state_error(state, t,
                  "Unable to convert '%s' for field '%s'.",
                  t->number, state->field->name);
            }
            STRUCT_MEMBER(double, msg, state->field->offset) = val;
            return STATE_OPEN;
          }
          break;

        default:
          return state_error(state, t,
              "'%s' is not a numeric field.", state->field->name);
          break;
      }
      break;

    default:
      return state_error(state, t,
                         "Expected value; found '%s' instead.",
                         token2txt(t));
      break;
  }
}

static StateId(* states[])(State *, Token *) = {
  [STATE_OPEN] = state_open,
  [STATE_ASSIGNMENT] = state_assignment,
  [STATE_VALUE] = state_value
};

static ProtobufCMessage *
text_format_parse(const ProtobufCMessageDescriptor *descriptor,
    Scanner *scanner,
    char **error_txt,
    ProtobufCAllocator *allocator)
{
  Token token;
  State state;
  StateId state_id;
  ProtobufCMessage *msg;

  if (!allocator) {
    allocator = &protobuf_c_default_allocator;
  }
  *error_txt = NULL;

  state_id = STATE_OPEN;
  if (!state_init(&state, scanner, descriptor, allocator)) {
    return NULL;
  }

  while (state_id != STATE_DONE) {
    token = scan(scanner, allocator);
    if (token.id == TOK_MALLOC_ERR) {
      token_free(&token, allocator);
      break;
    }
    state_id = states[state_id](&state, &token);
    token_free(&token, allocator);
  }

  scanner_free(scanner, allocator);
  msg = state.msgs[0];
  if (state.error) {
    *error_txt = state.error_str;
  }
  state_free(&state);
  return msg;
}

ProtobufCMessage *
text_format_from_file(const ProtobufCMessageDescriptor *descriptor,
    FILE *msg_file,
    char **error_txt,
    ProtobufCAllocator *allocator)
{
  Scanner scanner;

  scanner_init_file(&scanner, msg_file);
  return text_format_parse(descriptor, &scanner, error_txt, allocator);
}

ProtobufCMessage *
text_format_from_string(const ProtobufCMessageDescriptor *descriptor,
    char *msg,
    char **error_txt,
    ProtobufCAllocator *allocator)
{
  Scanner scanner;

  scanner_init_string(&scanner, msg);
  return text_format_parse(descriptor, &scanner, error_txt, allocator);
}
