#!/bin/bash

if [[ -z "$srcdir" ]]; then
  srcdir=.
fi

### Tutorial Addressbook message.
rm -f t/addressbook.c.text t/addressbook.protoc-c.text
./t/c-dump $srcdir/t/addressbook.data > t/addressbook.c.text
protoc-c --decode=tutorial.AddressBook -I $srcdir \
  $srcdir/t/addressbook.proto \
  < $srcdir/t/addressbook.data > t/addressbook.protoc-c.text

if ! cmp t/addressbook.c.text t/addressbook.protoc-c.text; then
  exit 1
fi

### Tutorial Test message.
if ! ./t/c-dump2 $srcdir/t/tutorial_test.data \
    > t/tutorial_test.c.text; then
./t/c-dump2 $srcdir/t/tutorial_test.data
cat << EOF
ERROR: This shouldn't have failed.
./t/c-dump2 $srcdir/t/tutorial_test.data > t/tutorial_test.c.text
EOF
  exit 1
fi

echo -n "Testing broken malloc"
i=0
exit_code=1
# while [[ $exit_code -ne 0 && $i -lt 100 ]]; do
while [[ $i -lt 300 ]]; do
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
Debug: 
BROKEN_MALLOC=$i gdb ./t/c-dump
run $srcdir/t/addressbook.data > t/broken.text
EOF
      exit 1
    fi
  fi
  i=$(($i + 1))
  rm t/broken.text
done
echo
