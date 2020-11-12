#!/bin/sh

rm -rf tmp/new
mkdir tmp/new
for f in built/jm-v2.0/app/jacdac-c/services/*.o ; do arm-none-eabi-objdump -d $f > tmp/new/`basename $f`.txt ; done
diff -urN tmp/ok tmp/new

