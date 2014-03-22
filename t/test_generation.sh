#!/bin/bash

test -f t/moo.ab || ./t/add_person t/moo.ab
./t/c-dump t/moo.ab > t/moo.c.text
./t/dump_people t/moo.ab > t/moo.c++.text

cmp t/moo.c.text t/moo.c++.text
