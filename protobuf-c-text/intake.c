#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include "lexer-global.h"
#include "parser.h"

#define BUFS 1024


typedef struct yy_buffer_state *YY_BUFFER_STATE;
int             yylex(void);
YY_BUFFER_STATE yy_scan_string(const char *);
void            yy_delete_buffer(YY_BUFFER_STATE);


int main(int argc,char** argv)
{
  int n;
  int yv;
  char buf[BUFS+1];

  while ((n=read(fileno(stdin), buf, BUFS )) >  0) {
    buf[n]='\0';
    yy_scan_string(buf);
    while ((yv = yylex()) != 0) { 
      printf(" yylex(): %d ", yv);
      if (yv == BOOLEAN) {
        printf("yylval.boolean = %s\n", yylval.boolean? "true": "false");
      } else {
        printf("yylval.string = \"%s\"\n", yylval.string);
      }
    }

  }

}
