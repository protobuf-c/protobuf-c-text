# Protobuf Text Format Support for C

Python, C++ and Java protobufs support text format but C protobufs do not.
This is a project to try and fix that. The end goal is to merge this
into the protobuf-c project.

## Contents

The C++ protobuf addressbook examples are included as a way to generate
text output and generate test data.  It doesn't exercise everything
about the protobuf format so will need to be expanded.

The `c-dump.c` code is a first pass at looping throught the descriptor
structure.

The lex and yacc examples are included since lex/yacc might be an easy
way to write a text format parser and it's been a long while since I've
written lex/yacc code.

## Build targets

* `all`: Will build the four binaries.
* `test`: Will generate `moo.ab` with a single call to `add_person`
          which will let you make an entry into the address book.
          It will then generate text format protobufs using the C++
          and C protobuf code.
