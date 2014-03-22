#ifndef PROTOBUF_C_TEXT_H
#define PROTOBUF_C_TEXT_H

#include <google/protobuf-c/protobuf-c.h>

/* Output functions. */
extern char *text_format_to_string(ProtobufCMessage *m);

/* Intake functions. */
extern int text_format_from_string(ProtobufCMessage *m, char *msg);
extern int text_format_from_file(ProtobufCMessage *m, FILE *msg_file);

#endif /* PROTOBUF_C_TEXT_H */
