#!/bin/bash

test -f t/moo.ab || ./t/add_person t/moo.ab
./t/c-dump t/moo.ab
./t/dump_people t/moo.ab
