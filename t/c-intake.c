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

  text_format_from_file(NULL, stdin);
}
