/* c-basic-offset: 2; tab-width: 8; indent-tabs-mode: nil; mode: c
 * vi: set shiftwidth=2 tabstop=8 expandtab filetype=c:
 * :indentSize=2:tabSize=8:noTabs=true:mode=c:
 */

/** \file
 * Routines to parse text format protobufs.
 *
 * This file contains the internal support functions as well as the
 * exported functions which are used to parse text format protobufs
 * into C protobuf data types.
 *
 * Note that this file must first be pre-processed with \c re2c.  The
 * build system does this for you, but the manual step is as follows:
 *
 * \code
 * re2c -s -o protobuf-c-text/parse.c protobuf-c-text/parse.re
 * \endcode
 *
 * \author Kevin Lyda <kevin@ie.suberic.net>
 * \date   March 2014
 */

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <protobuf-c/protobuf-c.h>
#include "protobuf-c-text.h"
#include "protobuf-c-util.h"
#include "config.h"

/** \defgroup utility Utility functions
 * \ingroup internal
 * @{
 */

/** A realloc implementation using ProtobufCAllocator functions.
 *
 * Similar to \c realloc, but using ProtobufCAllocator functions to do the
 * memory manipulations.
 *
 * \param[in,out] ptr Memory to realloc.
 * \param[in] old_size The size of ptr before realloc.
 * \param[in] size The desired size of ptr after realloc.
 * \param[in] allocator The functions to use to achieve this.
 * \return \c NULL on realloc failure - note \c ptr isn't freed in this
 *         case.  The new, \c size sized pointer (and \c ptr is freed in
 *         this case).
 */
static void *
local_realloc(void *ptr,
    size_t old_size,
    size_t size,
    ProtobufCAllocator *allocator)
{
  void *tmp;

  tmp = PBC_ALLOC(size);
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

  PBC_FREE(ptr);
  return tmp;
}

/** @} */  /* End of utility group. */

/** \defgroup lexer Routines related to lexing text format protobufs
 * \ingroup internal
 * @{
 */

/** Token types.
 *
 * Types of tokens found by scan(). These will be in the \c id
 * field of \c Token .
 */
typedef enum {
  TOK_EOF,        /**< End of file. */
  TOK_BAREWORD,   /**< A bare, unquoted single word. */
  TOK_OBRACE,     /**< An opening brace. */
  TOK_CBRACE,     /**< A closing brace. */
  TOK_COLON,      /**< A colon. */
  TOK_QUOTED,     /**< A quoted string. */
  TOK_NUMBER,     /**< A number. */
  TOK_BOOLEAN,    /**< The unquoted form of "true" and "false". */
  TOK_MALLOC_ERR  /**< A memory allocation error occurred. */
} TokenId;

/** A token and its value.
 *
 * A \c Token found by scan().  It contains the \c TokenId and the
 * value of the token found (if a value is relevant).
 */
typedef struct _Token {
  TokenId id;        /**< The kind of token. */
  union {
    char *number;    /**< \b TOK_NUMBER: string with the number. */
    char *bareword;  /**< \b TOK_BAREWORD: string with bareword in it. */
    ProtobufCBinaryData *qs; /**< \b TOK_QUOTED: Unescaped quoted string
                               with the quotes removed. */
    bool boolean;    /**< \b TOK_BOOLEAN: \c true or \c false . */
  };
} Token;

/** Converts a Token to a string based on its type.
 *
 * Provides a string summary of the type of token; used for error messages.
 *
 * \param[in] t The token.
 * \return A string representation of the token. This is a char * on the stack.
 *         Do not modify or free it.
 */
static const char *
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

/** Frees memory allocated in a \c Token instance.
 *
 * Frees memory allocated in a \c Token instance, but not
 * the \c Token itself.
 *
 * \param[in] t The token.
 * \param[in] allocator The memory allocator functions.
 */
static void
token_free(Token *t, ProtobufCAllocator *allocator)
{
  switch (t->id) {
    case TOK_BAREWORD:
      PBC_FREE(t->bareword);
      break;
    case TOK_QUOTED:
      PBC_FREE(t->qs->data);
      PBC_FREE(t->qs);
      break;
    case TOK_NUMBER:
      PBC_FREE(t->number);
      break;
    default:
      break;
  }
}

/** Maintains state for successive calls to scan() .
 *
 * This structure is used by the scanner to maintain state.
 */
typedef struct _Scanner {
  unsigned char *cursor; /**< Where we are in the \c buffer. */
  unsigned char *marker; /**< Used for backtracking. */
  unsigned char *buffer; /**< The buffer holding the data being parsed. */
  unsigned char *limit;  /**< Where the buffer ends. */
  unsigned char *token;  /**< Pointer to the start of the current token. */
  FILE *f;  /**< For file scanners, this is the input source.  Data read
              from it is put in \c buffer. */
  int line; /**< Current line number being parsed. Used for error
              reporting. */
} Scanner;

/** Initialise a \c Scanner from a \c FILE
 *
 * The resulting \c Scanner will load input from a \c FILE.
 *
 * \param[in,out] scanner The state struct for the scanner.
 * \param[in] f \c FILE to read input from.
 */
static void
scanner_init_file(Scanner *scanner, FILE *f)
{
  memset(scanner, 0, sizeof(Scanner));
  scanner->f = f;
  scanner->line = 1;
}

/** Initialise a \c Scanner from a string.
 *
 * The resulting \c Scanner will load input from a string.
 *
 * \param[in,out] scanner The state struct for the scanner.
 * \param[in] buf String to get input from.
 */
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

/** Free data internal to the \c Scanner instance.
 *
 * \param[in,out] scanner The state struct for the scanner.
 * \param[in] allocator Allocator functions.
 */
static void
scanner_free(Scanner *scanner, ProtobufCAllocator *allocator)
{
  if (scanner->f && scanner->buffer)
    PBC_FREE(scanner->buffer);
  scanner->buffer = NULL;
}

/** Unescape string.
 *
 * Remove escape sequences from a string and replace them with the
 * actual characters.
 *
 * \param[in] src String to unescape.
 * \param[in] len Length of string to unescape.
 * \param[in] allocator Allocator functions.
 * \return A ProtobufCBinaryData pointer with the unescaped data.
 *         Note this must be freed with the ProtobufCAllocator
 *         allocator you called this with.
 */
static ProtobufCBinaryData *
unesc_str(unsigned char *src, int len, ProtobufCAllocator *allocator)
{
  ProtobufCBinaryData *dst_pbbd;
  unsigned char *dst;
  int i = 0, dst_len = 0;
  unsigned char oct[4];

  dst_pbbd = PBC_ALLOC(sizeof(ProtobufCBinaryData));
  dst = PBC_ALLOC(len + 1);
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
  PBC_FREE(dst);
  PBC_FREE(dst_pbbd);
  return NULL;
}

/** Amount of data to read from a file each time. */
#define CHUNK 4096

/** Function to request more data from input source in \c Scanner.
 *
 * In the case of a string being the input source for \c Scanner,
 * nothing happens. For a \c FILE backed \c Scanner, a \c CHUNK's
 * worth of data is read from the \c FILE.
 *
 * \param[in,out] scanner The state struct for the scanner.
 * \param[in] allocator Allocator functions.
 * \return Returns the success of the function:
 *         - -1: Memory allocation failure.
 *         - 0: No more input added.
 *         - >0: Input added.
 */
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
    buf = PBC_ALLOC(len);
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
    PBC_FREE(scanner->buffer);
    scanner->buffer = buf;
    scanner->marker = buf;
  }

  return scanner->limit >= scanner->cursor? 1: 0;
}

/** Return the token. */
#define RETURN(tt) { t.id = tt; return t; }
/** Retrieves more input if available. */
#define YYFILL(n) { fill_result = fill(scanner, allocator); \
                    if (fill_result <= 0) \
                      RETURN((fill_result == -1? TOK_MALLOC_ERR: TOK_EOF)); }

/** Generated lexer.
 *
 * The guts of the parser generated by \c re2c.
 *
 * \param[in,out] scanner The state struct for the scanner.
 * \param[in] allocator Allocator functions.
 * \return Returns the next \c Token it finds.
 */
static Token
scan(Scanner *scanner, ProtobufCAllocator *allocator)
{
  Token t;
  int fill_result;

token_start:
  scanner->token = scanner->cursor;

  /* I don't think multiline strings are allowed.  If I'm wrong,
   * the QS re should be ["] (EQ|[^"]|NL)* ["]; */

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
  QS = ["] (EQ|[^"])* ["];
  WS = [ \t];

  I | F       {
                t.number = PBC_ALLOC((scanner->cursor - scanner->token) + 1);
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
                t.bareword = PBC_ALLOC((scanner->cursor - scanner->token) + 1);
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
  "{"         { RETURN(TOK_OBRACE); }
  "}"         { RETURN(TOK_CBRACE); }
  ":"         { RETURN(TOK_COLON); }
  WS          { goto token_start; }
  NL          { scanner->line++; goto token_start; }
  "\000"      { RETURN(TOK_EOF); }
  */
}

/** @} */  /* End of lexer group. */

/** \defgroup state Routines that define a simple finite state machine
 * \ingroup internal
 * @{
 */

/** StateId enumeration.
 *
 * A list of states for the FSM.
 */
typedef enum {
  STATE_OPEN,       /**< Ready to start a new statement or close
                      a nested message. */
  STATE_ASSIGNMENT, /**< Ready to assign a scalar or a nested message. */
  STATE_VALUE,      /**< Assign the scalar. */
  STATE_DONE        /**< Nothing more to read or there's been an error. */
} StateId;

/** Max size of an error message. */
#define STATE_ERROR_STR_MAX 160

/** Maintain state for the FSM.
 *
 * Tracks the current state of the FSM.
 */
typedef struct {
  Scanner *scanner;         /**< Tracks state for the scanner. */
  const ProtobufCFieldDescriptor *field;  /**< After finding a
                          \b TOK_BAREWORD in a \b STATE_OPEN \c field
                          is set to the field in the message that
                          matches that bareword. */
  int current_msg;          /**< Index on the message stack of the
                              current message. */
  int max_msg;              /**< Size of the message stack. */
  ProtobufCMessage **msgs;  /**< The message stack.  As nested messages
                              are found, they're put here. */
  ProtobufCAllocator *allocator;  /**< allocator functions. */
  int error;                /**< Notes an error has occurred. */
  char *error_str;          /**< Text of error. */
} State;

/** Initialise a \c State struct.
 * \param[in,out] state A state struct pointer - the is the state
 *                      for the FSM.
 * \param[in,out] scanner The state struct for the scanner.
 * \param[in] descriptor Message descriptor.
 * \param[in] allocator Allocator functions.
 * \return Success (1) or failure (0). Failure is due to out of
 *         memory errors.
 */
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
  state->error_str = ST_ALLOC(STATE_ERROR_STR_MAX);
  state->msgs = ST_ALLOC(10 * sizeof(ProtobufCMessage *));
  state->max_msg = 10;
  msg = ST_ALLOC(descriptor->sizeof_message);
  if (!state->msgs || !msg || !state->error_str) {
    ST_FREE(state->error_str);
    ST_FREE(state->msgs);
    ST_FREE(msg);
    return 0;
  }
  descriptor->message_init(msg);
  state->msgs[0] = msg;

  return 1;
}

/** Free internal data in a \c State struct.
 *
 * Frees allocated data within the \c State instance. Note that the
 * \c State instance itself is not freed and that the \c State instance
 * contains a pointer to the \c ProtobufCAllocator allocator that
 * was passed in state_init().
 *
 * Note also that the \c error_str element is only freed if there hasn't
 * been an error.  If there has been an error, the responsibility falls
 * on the caller to free \c error_str .
 *
 * \param[in,out] state A state struct pointer.
 */
static void
state_free(State *state)
{
  if (!state->error) {
    ST_FREE(state->error_str);
  }
  ST_FREE(state->msgs);
}

/*
 * Helper function to handle errors.
 */
/** Handle an error in the FSM.
 *
 * At any point in the FSM if an error is encounters, call this function
 * with an explination of the error - and return the resulting value.
 *
 * \param[in,out] state A state struct pointer.
 * \param[in] t The \c Token to process.
 * \param[in] error_fmt printf style format string for the error message.
 * \param[in] ... Arguments for \c error_fmt .
 * \return This will always return \c STATE_DONE .
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
  error_idx = snprintf(state->error_str, STATE_ERROR_STR_MAX,
      "Error found on line %d.\n", state->scanner->line);

  if (error_idx < STATE_ERROR_STR_MAX) {
    va_start(args, error_fmt);
    vsnprintf(state->error_str + error_idx, STATE_ERROR_STR_MAX - error_idx,
        error_fmt, args);
    va_end(args);
  }

  return STATE_DONE;
}

/** Expect an element name (bareword) or a closing brace.
 *
 * Initial state, and state after each assignment completes (or a message
 * assignment starts. If a bareword is found, go into \c STATE_ASSIGNMENT
 * and if a closing brace is found, go into \c STATE_DONE.
 *
 * If something else is found or if there are no more messages on the stack
 * (in other words, we're already at the base message), call and return
 * with state_error().
 *
 * \param[in,out] state A state struct pointer.
 * \param[in] t The \c Token to process.
 * \return A StateID value.
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

/** Expect a colon or opening brace.
 *
 * The state where we expect an assignment.
 *
 * \param[in,out] state A state struct pointer.
 * \param[in] t The \c Token to process.
 * \return A StateID value.
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
        ProtobufCMessage **tmp;
        size_t n_members;

        /* Don't assign over an existing message. */
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

        /* Allocate space for the repeated message list. */
        if (state->field->label == PROTOBUF_C_LABEL_REPEATED) {
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
          tmp[n_members - 1] = NULL;
        }

        /* Create and push a new message on the message stack. */
        state->current_msg++;
        if (state->current_msg == state->max_msg) {
          ProtobufCMessage **tmp_msgs;

          state->max_msg += 10;
          tmp_msgs = local_realloc(
              state->msgs, (state->current_msg) * sizeof(ProtobufCMessage *),
              (state->max_msg) * sizeof(ProtobufCMessage *),
              state->allocator);
          if (!tmp_msgs) {
            return state_error(state, t, "Malloc failure.");
          }
          state->msgs = tmp_msgs;
        }
        state->msgs[state->current_msg]
          = ST_ALLOC(((ProtobufCMessageDescriptor *)
               state->field->descriptor)->sizeof_message);
        if (!state->msgs[state->current_msg]) {
          return state_error(state, t, "Malloc failure.");
        }
        ((ProtobufCMessageDescriptor *)state->field->descriptor)
          ->message_init(state->msgs[state->current_msg]);

        /* Assign the message just created. */
        if (state->field->label == PROTOBUF_C_LABEL_REPEATED) {
          tmp[n_members - 1] = state->msgs[state->current_msg];
          return STATE_OPEN;
        } else {
          STRUCT_MEMBER(ProtobufCMessage *, msg, state->field->offset)
            = state->msgs[state->current_msg];
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

/** Expect a quoted string, enum (bareword) or boolean.
 *
 * Assign the value in \c Token to the field we identified in the
 * state_open() call.  This function is huge in order to handle the
 * variety of data types and the struct offset math required to manipulate
 * them.
 *
 * \param[in,out] state A state struct pointer.
 * \param[in] t The \c Token to process.
 * \return A StateID value.
 */
static StateId
state_value(State *state, Token *t)
{
  ProtobufCMessage *msg;
  size_t n_members;
  char *end;
  int64_t val;

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
          pbbd[n_members - 1].data = ST_ALLOC(t->qs->len);
          if (!pbbd[n_members - 1].data) {
            return state_error(state, t, "Malloc failure.");
          }
          memcpy(pbbd[n_members - 1].data, t->qs->data, t->qs->len);
          pbbd[n_members - 1].len = t->qs->len;
          return STATE_OPEN;
        } else {
          pbbd = STRUCT_MEMBER_PTR(ProtobufCBinaryData, msg,
              state->field->offset);
          pbbd->data = ST_ALLOC(t->qs->len);
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
          s[n_members - 1] = ST_ALLOC(t->qs->len + 1);
          if (!s[n_members - 1]) {
            return state_error(state, t, "Malloc failure.");
          }
          memcpy(s[n_members - 1], t->qs->data, t->qs->len);
          s[n_members - 1][t->qs->len] = '\0';
          return STATE_OPEN;
        } else {
          unsigned char *s;

          s = ST_ALLOC(t->qs->len + 1);
          if (!s) {
            return state_error(state, t, "Malloc failure.");
          }
          memcpy(s, t->qs->data, t->qs->len);
          s[t->qs->len] = '\0';
          STRUCT_MEMBER(unsigned char *, msg, state->field->offset) = s;
          return STATE_OPEN;
        }

      }
      return state_error(state, t,
          "'%s' is not a string or byte field.", state->field->name);
      break;

    case TOK_NUMBER:
      switch (state->field->type) {
        case PROTOBUF_C_TYPE_INT32:
        case PROTOBUF_C_TYPE_UINT32:
        case PROTOBUF_C_TYPE_FIXED32:
          val = strtoul(t->number, &end, 10);
          if (*end != '\0' || val > (uint64_t)UINT32_MAX) {
            return state_error(state, t,
                "Unable to convert '%s' for field '%s'.",
                t->number, state->field->name);
          }
          if (state->field->label == PROTOBUF_C_LABEL_REPEATED) {
            uint32_t *vals;

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
            vals[n_members - 1] = (uint32_t)val;
            return STATE_OPEN;
          } else {
            STRUCT_MEMBER(uint32_t, msg, state->field->offset) = (uint32_t)val;
            return STATE_OPEN;
          }
          break;

        case PROTOBUF_C_TYPE_SINT32:
        case PROTOBUF_C_TYPE_SFIXED32:
          val = strtol(t->number, &end, 10);
          if (*end != '\0' || val < INT32_MIN || val > INT32_MAX) {
            return state_error(state, t,
                "Unable to convert '%s' for field '%s'.",
                t->number, state->field->name);
          }
          if (state->field->label == PROTOBUF_C_LABEL_REPEATED) {
            int32_t *vals;

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
            vals[n_members - 1] = (uint32_t)val;
            return STATE_OPEN;
          } else {
            STRUCT_MEMBER(int32_t, msg, state->field->offset) = val;
            return STATE_OPEN;
          }
          break;

        case PROTOBUF_C_TYPE_INT64:
        case PROTOBUF_C_TYPE_UINT64:
        case PROTOBUF_C_TYPE_FIXED64:
          val = strtoull(t->number, &end, 10);
          if (*end != '\0') {
            return state_error(state, t,
                "Unable to convert '%s' for field '%s'.",
                t->number, state->field->name);
          }
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
            vals[n_members - 1] = val;
            return STATE_OPEN;
          } else {
            STRUCT_MEMBER(uint64_t, msg, state->field->offset) = val;
            return STATE_OPEN;
          }
          break;

        case PROTOBUF_C_TYPE_SINT64:
        case PROTOBUF_C_TYPE_SFIXED64:
          val = strtoll(t->number, &end, 10);
          if (*end != '\0') {
            return state_error(state, t,
                "Unable to convert '%s' for field '%s'.",
                t->number, state->field->name);
          }
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
            vals[n_members - 1] = val;
            return STATE_OPEN;
          } else {
            STRUCT_MEMBER(int64_t, msg, state->field->offset) = val;
            return STATE_OPEN;
          }
          break;

        case PROTOBUF_C_TYPE_FLOAT:
          {
            float val, *vals;

            errno = 0;
            val = strtof(t->number, &end);
            if (*end != '\0' || errno == ERANGE) {
              return state_error(state, t,
                  "Unable to convert '%s' for field '%s'.",
                  t->number, state->field->name);
            }
            if (state->field->label == PROTOBUF_C_LABEL_REPEATED) {
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
              vals[n_members - 1] = val;
              return STATE_OPEN;
            } else {
              STRUCT_MEMBER(float, msg, state->field->offset) = val;
              return STATE_OPEN;
            }
          }
          break;

        case PROTOBUF_C_TYPE_DOUBLE:
          {
            double val,*vals;

            errno = 0;
            val = strtod(t->number, &end);
            if (*end != '\0' || errno == ERANGE) {
              return state_error(state, t,
                  "Unable to convert '%s' for field '%s'.",
                  t->number, state->field->name);
            }
            if (state->field->label == PROTOBUF_C_LABEL_REPEATED) {
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
              vals[n_members - 1] = val;
              return STATE_OPEN;
            } else {
              STRUCT_MEMBER(double, msg, state->field->offset) = val;
              return STATE_OPEN;
            }
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

/** Table of states and actions.
 *
 * This is a table of each state and the action to take when in it.
 */
static StateId(* states[])(State *, Token *) = {
  [STATE_OPEN] = state_open,
  [STATE_ASSIGNMENT] = state_assignment,
  [STATE_VALUE] = state_value
};

/** @} */  /* End of state group. */

/** \defgroup base-parse Base parsing function
 * \ingroup internal
 * @{
 */

/** Base function for the API functions.
 *
 * The API functions take a string or a \c FILE.  This function takes an
 * appropriately initialised \c Scanner instead.  After that it works
 * the same as the protobuf_c_text_from* family of functions.
 *
 * \param[in] descriptor a \c ProtobufCMessageDescriptor of a message you
 *                       want to deserialise.
 * \param[in] scanner A \c Scanner which will be used by the FSM to parse
 *                    the text format protobuf.
 * \param[in,out] result A \c ProtobufCTextError instance to record any
 *                       errors.  It is not an option to pass \c NULL for
 *                       this and it must be checked for errors.
 * \param[in] allocator Allocator functions.
 * \return \c NULL on error. A \c ProtobufCMessage representation of the
 *         text format protobuf on success.
 */
static ProtobufCMessage *
protobuf_c_text_parse(const ProtobufCMessageDescriptor *descriptor,
    Scanner *scanner,
    ProtobufCTextError *result,
    ProtobufCAllocator *allocator)
{
  Token token;
  State state;
  StateId state_id;
  ProtobufCMessage *msg = NULL;

  result->error_txt = NULL;
  result->complete = -1;  /* -1 means the check wasn't performed. */

  state_id = STATE_OPEN;
  if (!state_init(&state, scanner, descriptor, allocator)) {
    return NULL;
  }

  while (state_id != STATE_DONE) {
    token = scan(scanner, allocator);
    if (token.id == TOK_MALLOC_ERR) {
      token_free(&token, allocator);
      state_error(&state, &token, "String unescape or malloc failure.");
      break;
    }
    state_id = states[state_id](&state, &token);
    token_free(&token, allocator);
  }

  scanner_free(scanner, allocator);
  if (state.error) {
    result->error_txt = state.error_str;
    if (msg) {
      protobuf_c_message_free_unpacked(state.msgs[0], allocator);
    }
  } else {
    msg = state.msgs[0];
#ifdef HAVE_PROTOBUF_C_MESSAGE_CHECK
    result->complete = protobuf_c_message_check(msg);
#endif
  }
  state_free(&state);
  return msg;
}

/** @} */  /* End of base-parse group. */

/* See .h file for API docs. */

ProtobufCMessage *
protobuf_c_text_from_file(const ProtobufCMessageDescriptor *descriptor,
    FILE *msg_file,
    ProtobufCTextError *result,
    ProtobufCAllocator *allocator)
{
  Scanner scanner;

  scanner_init_file(&scanner, msg_file);
  return protobuf_c_text_parse(descriptor, &scanner, result, allocator);
}

ProtobufCMessage *
protobuf_c_text_from_string(const ProtobufCMessageDescriptor *descriptor,
    char *msg,
    ProtobufCTextError *result,
    ProtobufCAllocator *allocator)
{
  Scanner scanner;

  scanner_init_string(&scanner, msg);
  return protobuf_c_text_parse(descriptor, &scanner, result, allocator);
}
