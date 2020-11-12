#!/bin/sh

for f in "$@" ; do
  s=`arm-none-eabi-objdump "$f" -h | awk '/\.(ro)?data/ { print "-j " $2 }' | sort | uniq`
  arm-none-eabi-objdump -d "$f"
  if [ "X$s" != "X" ]; then
    arm-none-eabi-objdump -s $s "$f"
  fi
done
