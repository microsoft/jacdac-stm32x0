#!/bin/sh

set -x
set -e

scp pibridge.c pi@192.168.0.131:spi/
ssh pi@192.168.0.131 'cd spi && gcc -W -Wall pibridge.c -lgpiod -lpthread && sudo ./a.out'
