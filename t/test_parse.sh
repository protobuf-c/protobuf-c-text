#!/bin/bash

rm -f t/addressbook.c.data

if [[ ! -f t/addressbook.c.text ]]; then
  exit 77
fi
if ! ./t/c-parse t/addressbook.c.data < t/addressbook.c.text; then
  exit 1;
fi

if ! cmp t/addressbook.c.data $srcdir/t/addressbook.data; then
  exit 1;
fi

for text in $srcdir/t/broken/*.text; do
  ./t/c-parse t/broken_parse.data < $text > t/broken_parse.out
  if ! grep -q ERROR t/broken_parse.out; then
    echo "./t/c-parse t/broken_parse.data < $text"
    echo "This didn't fail as expected."
    exit 1;
  fi
  if [[ -f t/broken_parse.data ]]; then
    echo "Parse for $text worked but shouldn't have."
    exit 1;
  fi
  rm -f t/broken_parse.data
done
