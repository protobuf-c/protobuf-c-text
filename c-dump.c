#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <google/protobuf-c/protobuf-c.h>
#include "addressbook.pb-c.h"

#define STRUCT_MEMBER_P(struct_p, struct_offset) \
      ((void *) ((uint8_t *) (struct_p) + (struct_offset)))

#define STRUCT_MEMBER(member_type, struct_p, struct_offset) \
      (*(member_type *) STRUCT_MEMBER_P((struct_p), (struct_offset)))

#define STRUCT_MEMBER_PTR(member_type, struct_p, struct_offset) \
      ((member_type *) STRUCT_MEMBER_P((struct_p), (struct_offset)))

#define CHUNK 1024

Tutorial__AddressBook *ab;

Tutorial__AddressBook *
read_addressbook(char *filename)
{
  FILE *f;
  int bufsize = 0, bytes, bytes_total = 0;
  char *buf = NULL;
  Tutorial__AddressBook *ab;

  f = fopen(filename, "r");
  if (!f) {
    printf("Can't open addressbook file '%s'.\n", filename);
    exit(1);
  }
  do {
    bufsize += CHUNK;
    buf = realloc(buf, bufsize);
    bytes = fread(buf + bytes_total, 1, CHUNK, f);
    bytes_total += bytes;
  } while (bytes == CHUNK);
  ab = tutorial__address_book__unpack(NULL, bytes_total, buf);
  free(buf);
  return ab;
}

void
text_format_to_string_int(int level, ProtobufCMessage *m,
    const ProtobufCMessageDescriptor *d)
{
  int i;
  const ProtobufCFieldDescriptor *f;

  printf("Name: %s\n", d->short_name);
  f = d->fields;
  for (i = 0; i < d->n_fields; i++) {
    switch (f[i].label) {
      case PROTOBUF_C_LABEL_REQUIRED:
        printf("required.\n");
        break;
      case PROTOBUF_C_LABEL_OPTIONAL:
        printf("optional (%d).\n",
            STRUCT_MEMBER(protobuf_c_boolean, m, f[i].quantifier_offset));
        break;
      case PROTOBUF_C_LABEL_REPEATED:
        printf("repeated (%zd).\n",
            *(size_t *)((uint8_t *)m + f[i].quantifier_offset));
        break;
    }

    if ((f[i].label == PROTOBUF_C_LABEL_REQUIRED)
        || ((f[i].label == PROTOBUF_C_LABEL_OPTIONAL)
          && *(protobuf_c_boolean *)((uint8_t *)m + f[i].quantifier_offset))
        || ((f[i].label == PROTOBUF_C_LABEL_REPEATED)
          && *(size_t *)((uint8_t *)m + f[i].quantifier_offset))) {
      switch (f[i].type) {
        case PROTOBUF_C_TYPE_INT32:
        case PROTOBUF_C_TYPE_UINT32:
        case PROTOBUF_C_TYPE_FIXED32:
          printf("%*s%s: %d\n",
              level, "", f[i].name,
              STRUCT_MEMBER(int, m, f[i].offset));
          break;
        case PROTOBUF_C_TYPE_SINT32:
        case PROTOBUF_C_TYPE_SFIXED32:
          break;
        case PROTOBUF_C_TYPE_INT64:
        case PROTOBUF_C_TYPE_UINT64:
        case PROTOBUF_C_TYPE_FIXED64:
          break;
        case PROTOBUF_C_TYPE_SINT64:
        case PROTOBUF_C_TYPE_SFIXED64:
          break;
        case PROTOBUF_C_TYPE_FLOAT:
          break;
        case PROTOBUF_C_TYPE_DOUBLE:
          break;
        case PROTOBUF_C_TYPE_BOOL:
          break;
        case PROTOBUF_C_TYPE_ENUM:
          break;
        case PROTOBUF_C_TYPE_STRING:
          printf("%*s%s: \"%s\"",
              level, "", f[i].name,
              STRUCT_MEMBER(char *, m, f[i].offset));
          break;
        case PROTOBUF_C_TYPE_BYTES:
          break;

        case PROTOBUF_C_TYPE_MESSAGE:
          if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
            int j;

            for (j = 0;
                j < *(size_t *)((uint8_t *)m + f[i].quantifier_offset);
                j++) {
              printf("%*s%s: {\n", level, "", f[i].name);
              text_format_to_string_int(level + 2,
                  STRUCT_MEMBER(ProtobufCMessage **, m, f[i].offset)[j],
                  (ProtobufCMessageDescriptor *)f[i].descriptor);
              printf("%*s}\n", level, "");
            }
          } else {
            printf("%*s%s: {\n", level, "", f[i].name);
            text_format_to_string_int(level + 2,
                STRUCT_MEMBER(ProtobufCMessage *, m, f[i].offset),
                (ProtobufCMessageDescriptor *)f[i].descriptor);
            printf("%*s}\n", level, "");
          }
          break;
        default:
          printf("unknown value\n");
      }

    }
  }
}

void
text_format_to_string(ProtobufCMessage *m)
{
  text_format_to_string_int(0, m, m->descriptor);
}

int
main(int argc, char *argv[])
{

  if (argc != 2) {
    printf("Must supply address book file.\n");
    exit(1);
  }

  ab = read_addressbook(argv[1]);
  printf("%lu %lu\n", (uint8_t *)&ab->person[0]->name - (uint8_t *)ab,
      sizeof(ProtobufCMessage));

  text_format_to_string((ProtobufCMessage *)ab);
  return 0;
}
