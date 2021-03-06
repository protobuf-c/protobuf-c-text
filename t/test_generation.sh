#!/bin/sh

if [ -z "$srcdir" ]; then
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
export BROKEN_MALLOC_SENTINAL=.broken_malloc
while [ $i -lt 300 ]; do
  echo -n .
  rm -f t/broken.text
  touch "$BROKEN_MALLOC_SENTINAL"
  BROKEN_MALLOC=$i ./t/c-dump $srcdir/t/addressbook.data \
    > t/broken.text
  exit_code=$?
  if [ $exit_code -ne 0 ]; then
    if ! $GREP ERROR t/broken.text > /dev/null 2>&1; then
      cat << EOF
ERROR: This should have failed.
BROKEN_MALLOC=$i ./t/c-dump $srcdir/t/addressbook.data \
Debug: 
BROKEN_MALLOC=$i gdb ./t/c-dump
run $srcdir/t/addressbook.data > t/broken.text
EOF
      rm "$BROKEN_MALLOC_SENTINAL"
      echo
      exit 1
    fi
  fi
  i=`expr $i + 1`
  rm t/broken.text
  if [ -f "$BROKEN_MALLOC_SENTINAL" ]; then
    rm "$BROKEN_MALLOC_SENTINAL"
    echo
    exit 0
  fi
done
echo
rm "$BROKEN_MALLOC_SENTINAL"
