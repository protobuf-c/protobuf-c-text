#ifndef PROTOBUF_C_UTIL_H
#define PROTOBUF_C_UTIL_H

/** \file
 * Internal utility header file.
 * Macros used by the generator and parser parts of the library.
 *
 * \author Kevin Lyda <kevin@ie.suberic.net>
 * \date   March 2014
 */

/** \defgroup internal Internal API
 *
 * These are the functions and data structures used internally.  They are
 * not exported and are not useable by users of \c libprotobuf-c-text.
 */

/* These are lifted from the protobuf-c lib */

/** Used to define STRUCT_MEMBER() and STRUCT_MEMBER_PTR(). */
#define STRUCT_MEMBER_P(struct_p, struct_offset) \
      ((void *) ((uint8_t *) (struct_p) + (struct_offset)))

/** Return a field from a message based on offset and type. */
#define STRUCT_MEMBER(member_type, struct_p, struct_offset) \
      (*(member_type *) STRUCT_MEMBER_P((struct_p), (struct_offset)))

/** Return a pointer to a field in a message based on offset and type. */
#define STRUCT_MEMBER_PTR(member_type, struct_p, struct_offset) \
      ((member_type *) STRUCT_MEMBER_P((struct_p), (struct_offset)))

#endif /* PROTOBUF_C_UTIL_H */
