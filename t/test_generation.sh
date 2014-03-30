#!/bin/bash

rm -f t/addressbook.c.text t/addressbook.protoc-c.text
./t/c-dump $srcdir/t/addressbook.data > t/addressbook.c.text
protoc-c --decode=tutorial.AddressBook -I $srcdir $srcdir/t/addressbook.proto \
  < $srcdir/t/addressbook.data > t/addressbook.protoc-c.text

cmp t/addressbook.c.text t/addressbook.protoc-c.text
