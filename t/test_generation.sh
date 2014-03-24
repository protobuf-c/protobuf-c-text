#!/bin/bash

test -f t/moo.ab || ./t/add_person t/moo.ab
./t/c-dump t/moo.ab > t/moo.c.text
protoc-c --decode=tutorial.AddressBook addressbook.proto \
  < t/moo.ab > t/moo.c++.text

cmp t/moo.c.text t/moo.c++.text
