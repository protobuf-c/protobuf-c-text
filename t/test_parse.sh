#!/bin/bash

if [[ -z "$srcdir" ]]; then
  srcdir=.
fi

### Addressbook message main tests.
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

### Tutorial Test message main tests.
rm -f t/tutorial_test.c.data

if [[ ! -f t/tutorial_test.c.text ]]; then
  exit 77
fi
if ./t/c-parse t/tutorial_test.c.data < t/tutorial_test.c.text > /dev/null; then
  echo "./t/c-parse t/tutorial_test.c.data < t/tutorial_test.c.text"
  echo "ERROR: should have failed."
  exit 1;
fi
if ! ./t/c-parse2 t/tutorial_test.c.data < t/tutorial_test.c.text; then
  echo "./t/c-parse2 t/tutorial_test.c.data < t/tutorial_test.c.text"
  echo "ERROR: should NOT have failed."
  exit 1;
fi

if ! cmp t/tutorial_test.c.data $srcdir/t/tutorial_test.data; then
  exit 1;
fi

### Malloc failure tests.
if [[ -z "$BROKEN_MALLOC_TEST" ]]; then
  exit 0
fi

echo -n "Testing broken malloc"
i=0
exit_code=1
#while [[ $exit_code -ne 0 && $i -lt 100 ]]; do
while [[ $i -lt 100 ]]; do
  echo -n .
  rm -f t/broken_parse.out t/broken_parse.data
  BROKEN_MALLOC=$i ./t/c-parse t/broken_parse.data \
    < t/addressbook.c.text > t/broken_parse.out
  exit_code=$?
  if [[ $exit_code -ne 0 ]]; then
    if ! grep -q ERROR t/broken_parse.out; then
      cat << EOF
ERROR: This should have failed.
BROKEN_MALLOC=$i ./t/c-parse t/broken_parse.data \
  < t/addressbook.c.text > t/broken_parse.out
EOF
      exit 1
    fi
  fi
  i=$(($i + 1))
done
echo
