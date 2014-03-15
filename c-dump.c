#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <google/protobuf-c/protobuf-c.h>
#include "addressbook.pb-c.h"

/* These are lifted from the protobuf-c lib */

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
  size_t j, quantifier_offset;
  const ProtobufCFieldDescriptor *f;
  ProtobufCEnumDescriptor *enumd;
  ProtobufCEnumValue *enumv;

  f = d->fields;
  for (i = 0; i < d->n_fields; i++) {
    /* Decide if something needs to be done for this field. */
    switch (f[i].label) {
      case PROTOBUF_C_LABEL_OPTIONAL:
        if (f[i].type == PROTOBUF_C_TYPE_STRING) {
          if (!STRUCT_MEMBER(char *, m, f[i].offset)
              || (STRUCT_MEMBER(char *, m, f[i].offset)
                == (char *)f[i].default_value)) {
            continue;
          }
        } else {
          if (!STRUCT_MEMBER(protobuf_c_boolean, m, f[i].quantifier_offset)) {
            continue;
          }
        }
        break;
      case PROTOBUF_C_LABEL_REPEATED:
        if (!STRUCT_MEMBER(size_t, m, f[i].quantifier_offset)) {
          continue;
        }
        break;
    }

    quantifier_offset = STRUCT_MEMBER(size_t, m, f[i].quantifier_offset);
    /* Field exists and has data, dump it. */
    switch (f[i].type) {
      case PROTOBUF_C_TYPE_INT32:
      case PROTOBUF_C_TYPE_UINT32:
      case PROTOBUF_C_TYPE_FIXED32:
        if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
          for (j = 0; quantifier_offset; j++) {
            printf("%*s%s: %u\n",
                level, "", f[i].name,
                STRUCT_MEMBER(uint32_t *, m, f[i].offset)[j]);
          }
        } else {
          printf("%*s%s: %u\n",
              level, "", f[i].name,
              STRUCT_MEMBER(uint32_t, m, f[i].offset));
        }
        break;
      case PROTOBUF_C_TYPE_SINT32:
      case PROTOBUF_C_TYPE_SFIXED32:
        if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
          for (j = 0; quantifier_offset; j++) {
            printf("%*s%s: %d\n",
                level, "", f[i].name,
                STRUCT_MEMBER(int32_t *, m, f[i].offset)[j]);
          }
        } else {
          printf("%*s%s: %d\n",
              level, "", f[i].name,
              STRUCT_MEMBER(int32_t, m, f[i].offset));
        }
        break;
      case PROTOBUF_C_TYPE_INT64:
      case PROTOBUF_C_TYPE_UINT64:
      case PROTOBUF_C_TYPE_FIXED64:
        if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
          for (j = 0; quantifier_offset; j++) {
            printf("%*s%s: %lu\n",
                level, "", f[i].name,
                STRUCT_MEMBER(uint64_t *, m, f[i].offset)[j]);
          }
        } else {
          printf("%*s%s: %lu\n",
              level, "", f[i].name,
              STRUCT_MEMBER(uint64_t, m, f[i].offset));
        }
        break;
      case PROTOBUF_C_TYPE_SINT64:
      case PROTOBUF_C_TYPE_SFIXED64:
        if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
          for (j = 0; quantifier_offset; j++) {
            printf("%*s%s: %ld\n",
                level, "", f[i].name,
                STRUCT_MEMBER(int64_t *, m, f[i].offset)[j]);
          }
        } else {
          printf("%*s%s: %ld\n",
              level, "", f[i].name,
              STRUCT_MEMBER(int64_t, m, f[i].offset));
        }
        break;
      case PROTOBUF_C_TYPE_FLOAT:
        if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
          for (j = 0; quantifier_offset; j++) {
            printf("%*s%s: %s\n",
                level, "", f[i].name,
                "TODO - what type to use for floats?");
          }
        } else {
          printf("%*s%s: %s\n",
              level, "", f[i].name,
              "TODO - what type to use for floats?");
        }
        break;
      case PROTOBUF_C_TYPE_DOUBLE:
        if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
          for (j = 0; quantifier_offset; j++) {
            printf("%*s%s: %s\n",
                level, "", f[i].name,
                "TODO - what type to use for doubles?");
          }
        } else {
          printf("%*s%s: %s\n",
              level, "", f[i].name,
              "TODO - what type to use for doubles?");
        }
        break;
      case PROTOBUF_C_TYPE_BOOL:
        if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
          for (j = 0; quantifier_offset; j++) {
            printf("%*s%s: %s\n",
                level, "", f[i].name,
                STRUCT_MEMBER(protobuf_c_boolean *, m, f[i].offset)[j]?
                "true": "false");
          }
        } else {
          printf("%*s%s: %s\n",
              level, "", f[i].name,
              STRUCT_MEMBER(protobuf_c_boolean, m, f[i].offset)?
              "true": "false");
        }
        break;
      case PROTOBUF_C_TYPE_ENUM:
        enumd = (ProtobufCEnumDescriptor *)f[i].descriptor;
        if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
          for (j = 0; quantifier_offset; j++) {
            enumv = protobuf_c_enum_descriptor_get_value(
                enumd, STRUCT_MEMBER(int *, m, f[i].offset)[j]);
            printf("%*s%s: %s\n",
                level, "", f[i].name,
                enumv? enumv->name: "unknown");
          }
        } else {
          enumv = protobuf_c_enum_descriptor_get_value(
              enumd, STRUCT_MEMBER(int, m, f[i].offset));
          printf("%*s%s: %s\n",
              level, "", f[i].name,
              enumv? enumv->name: "unknown");
        }
        break;
      case PROTOBUF_C_TYPE_STRING:
        if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
          for (j = 0; quantifier_offset; j++) {
            printf("%*s%s: \"%s\"\n",
                level, "", f[i].name,
                STRUCT_MEMBER(char **, m, f[i].offset)[j]);
          }
        } else {
          printf("%*s%s: \"%s\"\n",
              level, "", f[i].name,
              STRUCT_MEMBER(char *, m, f[i].offset));
        }
        break;
      case PROTOBUF_C_TYPE_BYTES:
        /* This isn't supported by protobuf-c. */
        printf("%*s%s: %s\n",
            level, "", f[i].name,
            "TODO");
        break;

      case PROTOBUF_C_TYPE_MESSAGE:
        /* Clarification: I think loops for repeat fields need to
         * be done here. */
        if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
          for (j = 0;
              j < STRUCT_MEMBER(size_t, m, f[i].quantifier_offset);
              j++) {
            printf("%*s%s {\n", level, "", f[i].name);
            text_format_to_string_int(level + 2,
                STRUCT_MEMBER(ProtobufCMessage **, m, f[i].offset)[j],
                (ProtobufCMessageDescriptor *)f[i].descriptor);
            printf("%*s}\n", level, "");
          }
        } else {
          printf("%*s%s {\n", level, "", f[i].name);
          text_format_to_string_int(level + 2,
              STRUCT_MEMBER(ProtobufCMessage *, m, f[i].offset),
              (ProtobufCMessageDescriptor *)f[i].descriptor);
          printf("%*s}\n", level, "");
        }
        break;
      default:
        printf("unknown value\n");
        break;
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

  text_format_to_string((ProtobufCMessage *)ab);
  return 0;
}
