#!/bin/sh

if test -f "$1" -a -f "$2" ; then
  hexdump -C "$1" > "$1".hex
  hexdump -C "$2" > "$2".hex
  diff -u "$1".hex "$2".hex
else
  echo "usage: $0 f1.bin f2.bin"
  exit 1
fi
