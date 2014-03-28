%include {
#include <stdlib.h>
#include <google/protobuf-c/protobuf-c.h>
#include "lexer.h"
#include "parser.h"
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

statement ::= BAREWORD COLON BAREWORD. {
           printf("Got a enum statement.\n");
           }
statement ::= BAREWORD COLON QUOTED. {
           printf("Got a string statement.\n");
           }
statement ::= BAREWORD COLON NUMBER. {
           printf("Got a number statement.\n");
           }
statement ::= BAREWORD COLON BOOLEAN. {
           printf("Got a boolean statement.\n");
           }
statement ::= message. {
           printf("Got a message statement.\n");
           }
statements ::= statements statement.
statements ::= .
