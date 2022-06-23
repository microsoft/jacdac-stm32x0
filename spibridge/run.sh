#!/bin/sh

set -x
set -e

scp pibridge-gpiod.c pi:spi/
ssh pi 'cd spi && gcc -W -Wall pibridge.c -lgpiod -pthread && ./a.out'
