#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <google/protobuf-c/protobuf-c.h>
#include "protobuf-c-text.h"
#include "protobuf-c-util.h"

typedef struct _ReturnString {
  int allocated;
  int pos;
  char *s;
} ReturnString;

static void rs_append(ReturnString *rs, int guess, const char *format, ...)
  __attribute__((format(printf, 3, 4)));
static void
rs_append(ReturnString *rs, int guess, const char *format, ...)
{
  va_list args;
  int added;

  if (rs->allocated - rs->pos < guess * 2) {
    rs->allocated += guess * 2;
    rs->s = realloc(rs->s, rs->allocated);
  }
  va_start(args, format);
  /* TODO: error check this. */
  added = vsnprintf(rs->s + rs->pos, rs->allocated - rs->pos, format, args);
  va_end(args);
  rs->pos += added;
}

static char *
esc_str(char *src, int len)
{
  int i, escapes = 0, dst_len = 0;
  unsigned char *dst;

  for (i = 0; i < len; i++) {
    if (!isprint(src[i])) {
      escapes++;
    }
  }
  dst = malloc((escapes * 4) + ((len - escapes) * 2) + 1);
  if (!dst) {
    return NULL;
  }

  for (i = 0; i < len; i++) {
    switch (src[i]) {
      /* Special cases. */
      case '\'':
        dst[dst_len++] = '\\';
        dst[dst_len++] = '\'';
        break;
      case '\"':
        dst[dst_len++] = '\\';
        dst[dst_len++] = '\"';
        break;
      case '\\':
        dst[dst_len++] = '\\';
        dst[dst_len++] = '\\';
        break;
      case '\n':
        dst[dst_len++] = '\\';
        dst[dst_len++] = 'n';
        break;
      case '\r':
        dst[dst_len++] = '\\';
        dst[dst_len++] = 'r';
        break;
      case '\t':
        dst[dst_len++] = '\\';
        dst[dst_len++] = 't';
        break;

      /* Escape with octal if !isprint. */
      default:
        if (!isprint(src[i])) {
          dst_len += sprintf(dst + dst_len, "\\%03o", src[i]);
        } else {
          dst[dst_len++] = src[i];
        }
        break;
    }
  }
  dst[dst_len] = '\0';

  return dst;
}

static void
text_format_to_string_int(ReturnString *rs,
    int level,
    ProtobufCMessage *m,
    const ProtobufCMessageDescriptor *d)
{
  int i;
  size_t j, quantifier_offset;
  double float_var;
  const ProtobufCFieldDescriptor *f;
  ProtobufCEnumDescriptor *enumd;
  const ProtobufCEnumValue *enumv;

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
            rs_append(rs, level + strlen(f[i].name) + 20,
                "%*s%s: %u\n",
                level, "", f[i].name,
                STRUCT_MEMBER(uint32_t *, m, f[i].offset)[j]);
          }
        } else {
          rs_append(rs, level + strlen(f[i].name) + 20,
              "%*s%s: %u\n",
              level, "", f[i].name,
              STRUCT_MEMBER(uint32_t, m, f[i].offset));
        }
        break;
      case PROTOBUF_C_TYPE_SINT32:
      case PROTOBUF_C_TYPE_SFIXED32:
        if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
          for (j = 0; quantifier_offset; j++) {
            rs_append(rs, level + strlen(f[i].name) + 20,
                "%*s%s: %d\n",
                level, "", f[i].name,
                STRUCT_MEMBER(int32_t *, m, f[i].offset)[j]);
          }
        } else {
          rs_append(rs, level + strlen(f[i].name) + 20,
              "%*s%s: %d\n",
              level, "", f[i].name,
              STRUCT_MEMBER(int32_t, m, f[i].offset));
        }
        break;
      case PROTOBUF_C_TYPE_INT64:
      case PROTOBUF_C_TYPE_UINT64:
      case PROTOBUF_C_TYPE_FIXED64:
        if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
          for (j = 0; quantifier_offset; j++) {
            rs_append(rs, level + strlen(f[i].name) + 20,
                "%*s%s: %lu\n",
                level, "", f[i].name,
                STRUCT_MEMBER(uint64_t *, m, f[i].offset)[j]);
          }
        } else {
          rs_append(rs, level + strlen(f[i].name) + 20,
              "%*s%s: %lu\n",
              level, "", f[i].name,
              STRUCT_MEMBER(uint64_t, m, f[i].offset));
        }
        break;
      case PROTOBUF_C_TYPE_SINT64:
      case PROTOBUF_C_TYPE_SFIXED64:
        if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
          for (j = 0; quantifier_offset; j++) {
            rs_append(rs, level + strlen(f[i].name) + 20,
                "%*s%s: %ld\n",
                level, "", f[i].name,
                STRUCT_MEMBER(int64_t *, m, f[i].offset)[j]);
          }
        } else {
          rs_append(rs, level + strlen(f[i].name) + 20,
              "%*s%s: %ld\n",
              level, "", f[i].name,
              STRUCT_MEMBER(int64_t, m, f[i].offset));
        }
        break;
      case PROTOBUF_C_TYPE_FLOAT:
        if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
          for (j = 0; quantifier_offset; j++) {
            float_var = STRUCT_MEMBER(float *, m, f[i].offset)[j];
            rs_append(rs, level + strlen(f[i].name) + 20,
                "%*s%s: %g\n",
                level, "", f[i].name,
                float_var);
          }
        } else {
          float_var = STRUCT_MEMBER(float, m, f[i].offset);
          rs_append(rs, level + strlen(f[i].name) + 20,
              "%*s%s: %g\n",
              level, "", f[i].name,
              float_var);
        }
        break;
      case PROTOBUF_C_TYPE_DOUBLE:
        if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
          for (j = 0; quantifier_offset; j++) {
            rs_append(rs, level + strlen(f[i].name) + 20,
                "%*s%s: %g\n",
                level, "", f[i].name,
                STRUCT_MEMBER(double *, m, f[i].offset)[j]);
          }
        } else {
          rs_append(rs, level + strlen(f[i].name) + 20,
              "%*s%s: %g\n",
              level, "", f[i].name,
              STRUCT_MEMBER(double, m, f[i].offset));
        }
        break;
      case PROTOBUF_C_TYPE_BOOL:
        if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
          for (j = 0; quantifier_offset; j++) {
            rs_append(rs, level + strlen(f[i].name) + 20,
                "%*s%s: %s\n",
                level, "", f[i].name,
                STRUCT_MEMBER(protobuf_c_boolean *, m, f[i].offset)[j]?
                "true": "false");
          }
        } else {
          rs_append(rs, level + strlen(f[i].name) + 20,
              "%*s%s: %s\n",
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
            rs_append(rs, level + strlen(f[i].name) + 20,
                "%*s%s: %s\n",
                level, "", f[i].name,
                enumv? enumv->name: "unknown");
          }
        } else {
          enumv = protobuf_c_enum_descriptor_get_value(
              enumd, STRUCT_MEMBER(int, m, f[i].offset));
          rs_append(rs, level + strlen(f[i].name) + 20,
              "%*s%s: %s\n",
              level, "", f[i].name,
              enumv? enumv->name: "unknown");
        }
        break;
      case PROTOBUF_C_TYPE_STRING:
        if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
          for (j = 0; quantifier_offset; j++) {
            unsigned char *escaped;

            escaped = esc_str(
                STRUCT_MEMBER(unsigned char **, m, f[i].offset)[j],
                strlen(STRUCT_MEMBER(unsigned char **, m, f[i].offset)[j]));
            rs_append(rs, level + strlen(f[i].name) + strlen(escaped) + 10,
                "%*s%s: \"%s\"\n", level, "", f[i].name, escaped);
            free(escaped);
          }
        } else {
          unsigned char *escaped;

          escaped = esc_str(STRUCT_MEMBER(unsigned char *, m, f[i].offset),
              strlen(STRUCT_MEMBER(unsigned char *, m, f[i].offset)));
          rs_append(rs, level + strlen(f[i].name) + strlen(escaped) + 10,
              "%*s%s: \"%s\"\n", level, "", f[i].name, escaped);
          free(escaped);
        }
        break;
      case PROTOBUF_C_TYPE_BYTES:
        if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
          for (j = 0; quantifier_offset; j++) {
            unsigned char *escaped;

            escaped = esc_str(
                STRUCT_MEMBER(ProtobufCBinaryData *, m, f[i].offset)[j].data,
                STRUCT_MEMBER(ProtobufCBinaryData *, m, f[i].offset)[j].len);
            rs_append(rs, level + strlen(f[i].name) + strlen(escaped) + 10,
                "%*s%s: \"%s\"\n", level, "", f[i].name, escaped);
            free(escaped);
          }
        } else {
          unsigned char *escaped;

          escaped = esc_str(
              STRUCT_MEMBER(ProtobufCBinaryData, m, f[i].offset).data,
              STRUCT_MEMBER(ProtobufCBinaryData, m, f[i].offset).len);
          rs_append(rs, level + strlen(f[i].name) + strlen(escaped) + 10,
              "%*s%s: \"%s\"\n", level, "", f[i].name, escaped);
          free(escaped);
        }
        break;

      case PROTOBUF_C_TYPE_MESSAGE:
        /* Clarification: I think loops for repeat fields need to
         * be done here. */
        if (f[i].label == PROTOBUF_C_LABEL_REPEATED) {
          for (j = 0;
              j < STRUCT_MEMBER(size_t, m, f[i].quantifier_offset);
              j++) {
            rs_append(rs, level + strlen(f[i].name) + 10,
                "%*s%s {\n", level, "", f[i].name);
            text_format_to_string_int(rs, level + 2,
                STRUCT_MEMBER(ProtobufCMessage **, m, f[i].offset)[j],
                (ProtobufCMessageDescriptor *)f[i].descriptor);
            rs_append(rs, level + 10,
                "%*s}\n", level, "");
          }
        } else {
          rs_append(rs, level + strlen(f[i].name) + 10,
              "%*s%s {\n", level, "", f[i].name);
          text_format_to_string_int(rs, level + 2,
              STRUCT_MEMBER(ProtobufCMessage *, m, f[i].offset),
              (ProtobufCMessageDescriptor *)f[i].descriptor);
          rs_append(rs, level + 10,
              "%*s}\n", level, "");
        }
        break;
      default:
        printf("unknown value\n");
        break;
    }

  }
}

char *
text_format_to_string(ProtobufCMessage *m)
{
  ReturnString rs = { 0, 0, NULL };

  text_format_to_string_int(&rs, 0, m, m->descriptor);

  return rs.s;
}
