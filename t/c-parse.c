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
  Tutorial__AddressBook *ab;
  size_t len;
  uint8_t *buf;
  FILE *out;
  char *error_txt;
  Tutorial__Short *shortmsg;

  ab = (Tutorial__AddressBook *)text_format_from_file(
      &tutorial__address_book__descriptor,
      stdin, &error_txt, &broken_allocator);
  if (error_txt) {
    printf("ERROR on import:\n%s", error_txt);
    free(error_txt);
    exit(1);
  }
  if (!ab) {
    printf("ERROR malloc failures.\n");
    exit(1);
  }

  len = tutorial__address_book__get_packed_size(ab);
  buf = malloc(len);
  tutorial__address_book__pack(ab, buf);
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

  tutorial__address_book__free_unpacked(ab, &broken_allocator);

  shortmsg = (Tutorial__Short *)text_format_from_string(
      &tutorial__short__descriptor,
      "id: 42", &error_txt, NULL);
  if (error_txt) {
    printf("ERROR on import:\n%s", error_txt);
    free(error_txt);
    exit(1);
  }
  if (!shortmsg) {
    printf("ERROR malloc failures.\n");
    exit(1);
  }
  tutorial__short__free_unpacked(shortmsg, NULL);

  exit(0);
}
