#!/bin/bash

if [[ ! -f t/addressbook.c.text ]]; then
  exit 77
fi
./t/c-intake t/addressbook.c.data < t/addressbook.c.text

cmp t/addressbook.c.data t/addressbook.data
