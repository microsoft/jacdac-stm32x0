#!/bin/sh

rm -rf tmp/new
mkdir tmp/new
./scripts/disasm.sh `find built/jm-v2.0/ -name '*.o' | sort` > tmp/new.txt
diff -u tmp/ok.txt tmp/new.txt

