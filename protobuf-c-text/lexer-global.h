#ifndef LEXER_GLOBAL_H
#define LEXER_GLOBAL_H

#include <stdbool.h>

#ifndef YYSTYPE
typedef union {
    char *string;
    bool boolean;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif


/* extern YYSTYPE yylval; */
YYSTYPE yylval; 

#endif /* LEXER_GLOBAL_H */
