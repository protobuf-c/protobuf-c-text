#!/bin/bash

if [[ -z "$srcdir" ]]; then
  srcdir=.
fi

rm -f t/addressbook.c.text t/addressbook.protoc-c.text
./t/c-dump $srcdir/t/addressbook.data > t/addressbook.c.text
protoc-c --decode=tutorial.AddressBook -I $srcdir \
  $srcdir/t/addressbook.proto \
  < $srcdir/t/addressbook.data > t/addressbook.protoc-c.text

if ! cmp t/addressbook.c.text t/addressbook.protoc-c.text; then
  exit 1
fi

echo -n "Testing broken malloc"
i=0
exit_code=1
# while [[ $exit_code -ne 0 && $i -lt 100 ]]; do
while [[ $i -lt 100 ]]; do
  echo -n .
  rm -f t/broken.text
  BROKEN_MALLOC=$i ./t/c-dump $srcdir/t/addressbook.data \
    > t/broken.text
  exit_code=$?
  if [[ $exit_code -ne 0 ]]; then
    if ! grep -q ERROR t/broken.text; then
      cat << EOF
ERROR: This should have failed.
BROKEN_MALLOC=$i ./t/c-dump $srcdir/t/addressbook.data \
  > t/broken.text
EOF
      exit 1
    fi
  fi
  i=$(($i + 1))
  rm t/broken.text
done
echo
