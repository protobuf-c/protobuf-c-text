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
  ProtobufCTextError tf_res;
  Tutorial__AddressBook *ab;
  size_t len;
  uint8_t *buf;
  FILE *out;
  Tutorial__Short *shortmsg;

  ab = (Tutorial__AddressBook *)protobuf_c_text_from_file(
      &tutorial__address_book__descriptor,
      stdin, &tf_res, &broken_allocator);
  if (tf_res.error_txt) {
    printf("ERROR on import:\n%s", tf_res.error_txt);
    free(tf_res.error_txt);
    exit(1);
  }
  if (!tf_res.complete) {
    printf("ERROR on import: Message not complete.\n");
    if (ab) {
      tutorial__address_book__free_unpacked(ab, &broken_allocator);
    }
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

  shortmsg = (Tutorial__Short *)protobuf_c_text_from_string(
      &tutorial__short__descriptor,
      "id: 42\ntruer: 7\nfalser: \"\t\"\n", &tf_res, NULL);
  if (tf_res.error_txt) {
    printf("ERROR on import:\n%s", tf_res.error_txt);
    free(tf_res.error_txt);
    exit(1);
  }
  if (!tf_res.complete) {
    printf("ERROR on import: Message not complete.\n");
    if (shortmsg) {
      tutorial__short__free_unpacked(shortmsg, &broken_allocator);
    }
    exit(1);
  }
  if (!shortmsg) {
    printf("ERROR malloc failures.\n");
    exit(1);
  }
  tutorial__short__free_unpacked(shortmsg, NULL);

  exit(0);
}
