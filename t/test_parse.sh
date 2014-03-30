#!/bin/bash

if [[ ! -f t/addressbook.c.text ]]; then
  exit 77
fi
./t/c-parse t/addressbook.c.data < t/addressbook.c.text

cmp t/addressbook.c.data $srcdir/t/addressbook.data
