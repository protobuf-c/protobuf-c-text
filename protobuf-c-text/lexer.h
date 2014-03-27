#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <google/protobuf-c/protobuf-c.h>

typedef struct _Scanner {
  unsigned char *cursor;
  unsigned char *buffer;
  unsigned char *limit;
  unsigned char *token;
  union {
    char *number;
    char *bareword;
    ProtobufCBinaryData *qs;
    bool boolean;
  };
  FILE *f;
} Scanner;

extern void scanner_init_file(Scanner *s, FILE *f);
extern void scanner_init_string(Scanner *s, char *buf);
extern int scan(Scanner *s);

#endif /* LEXER_H */
