#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <google/protobuf-c/protobuf-c.h>
#include "protobuf-c-text/protobuf-c-text.h"
#include "addressbook.pb-c.h"
#include "broken-alloc.h"

#define CHUNK 1024

int
main(int argc, char *argv[])
{
  Tutorial__Test *testmsg;
  size_t len;
  uint8_t *buf;
  FILE *out;
  char *errors;

  testmsg = (Tutorial__Test *)text_format_from_file(
            &tutorial__test__descriptor,
            stdin, &errors, &broken_allocator);
  if (errors) {
    printf("ERROR on import:\n%s", errors);
    free(errors);
    exit(1);
  }
  if (!testmsg) {
    printf("ERROR malloc failures.\n");
    exit(1);
  }

  len = tutorial__test__get_packed_size(testmsg);
  buf = malloc(len);
  tutorial__test__pack(testmsg, buf);
  if (argv[1]) {
    out = fopen(argv[1], "w");
    if (len != fwrite(buf, 1, len, out)) {
      printf("ERROR: Protobuf output file truncated.");
      exit(1);
    }
    fclose(out);
  } else {
    printf("ERROR: Name for protobuf output file not provided.");
    exit(1);
  }

  tutorial__test__free_unpacked(testmsg, &broken_allocator);

  exit(0);
}
