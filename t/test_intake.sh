#!/bin/bash

if [[ ! -f t/moo.c.text ]]; then
  exit 77
fi
./t/c-intake < t/moo.c.text
