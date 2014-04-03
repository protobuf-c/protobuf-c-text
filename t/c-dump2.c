#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <google/protobuf-c/protobuf-c.h>
#include "protobuf-c-text/protobuf-c-text.h"
#include "addressbook.pb-c.h"
#include "broken-alloc.h"

#define CHUNK 1024

Tutorial__Test *testmsg;

Tutorial__Test *
read_tutorial_test(char *filename)
{
  FILE *f;
  int bufsize = 0, bytes, bytes_total = 0;
  char *buf = NULL;
  Tutorial__Test *testmsg;

  f = fopen(filename, "r");
  if (!f) {
    printf("Can't open tutorial test message data file '%s'.\n", filename);
    exit(1);
  }
  do {
    bufsize += CHUNK;
    buf = realloc(buf, bufsize);
    bytes = fread(buf + bytes_total, 1, CHUNK, f);
    bytes_total += bytes;
  } while (bytes == CHUNK);
  testmsg = tutorial__test__unpack(NULL, bytes_total, buf);
  free(buf);
  return testmsg;
}

int
main(int argc, char *argv[])
{
  char *s;

  if (argc != 2) {
    printf("Must supply tutorial test message data file.\n");
    exit(1);
  }

  testmsg = read_tutorial_test(argv[1]);

  s = text_format_to_string((ProtobufCMessage *)testmsg, &broken_allocator);
  if (s) {
    printf("%s", s);
    free(s);
  } else {
    printf("ERROR: Failed to generate text format.\n");
  }

  return 0;
}
