# Protobuf Text Format Support for C

Python, C++ and Java protobufs support text format but C protobufs do not.
This is a project to try and fix that. The end goal is to merge this
into the protobuf-c project.

## Contents

The `protobuf-c-text/` directory has the code for the library.  Tests
are in `t/`.

## Dependencies

The `re2c` parser is required to generate the lexer (`parser.re`).
