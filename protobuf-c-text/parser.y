%token_type {int}

%include {
/* #include ".h" */
}

%syntax_error {
  printf("Syntax error!\n");
}

state ::= message.
message ::= BAREWORD OBRACE statements CBRACE. {
        printf("Got a message.\n");
        }

statements ::= BAREWORD COLON BAREWORD. {
           printf("Got a enum statement.\n");
           }
statements ::= BAREWORD COLON QUOTED. {
           printf("Got a string statement.\n");
           }
statements ::= BAREWORD COLON NUMBER. {
           printf("Got a number statement.\n");
           }
statements ::= BAREWORD COLON BOOLEAN. {
           printf("Got a boolean statement.\n");
           }
statements ::= BAREWORD COLON message. {
           printf("Got a message statement.\n");
           }
