[![Build Status](https://travis-ci.org/lyda/protobuf-c-text.png?branch=master)](https://travis-ci.org/lyda/protobuf-c-text)

# Protobuf Text Format Support for C

Python, C++ and Java protobufs support text format but C protobufs do not.
This is a project to try and fix that. The end goal is to merge this
into the protobuf-c project.

## Contents

The `protobuf-c-text/` directory has the code for the library.  Tests
are in `t/`.

## Dependencies

The `re2c` parser is required to generate the lexer (`parser.re`).

## Testing

The `t/c-*` programs use the `BROKEN_MALLOC` and `BROKEN_MALLOC_SEGV`
environment vars to control when and how malloc will fail.  The
`BROKEN_MALLOC` is set to the number of times for malloc to succeed until
it fails.  When the `BROKEN_MALLOC_SEGV` var is set the test program will
segfault on the first failure.  This is useful for tracking down errors.

Note that the error message will print out the `gdb` line and the `run`
command you need to issue to reproduce the error.
