#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <google/protobuf-c/protobuf-c.h>
#include "protobuf-c-text/protobuf-c-text.h"
#include "addressbook.pb-c.h"

#define CHUNK 1024

int
main(int argc, char *argv[])
{
  Tutorial__AddressBook ab = TUTORIAL__ADDRESS_BOOK__INIT;
  size_t len;
  uint8_t *buf;
  FILE *out;

  text_format_from_file((ProtobufCMessage *)&ab, stdin);

  len = tutorial__address_book__get_packed_size(&ab);
  buf = malloc(len);
  tutorial__address_book__pack(&ab, buf);
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
}
