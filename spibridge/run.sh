#!/bin/sh

set -x
set -e

host=pi@${RPI_HOST:-raspberrypi.local}

scp pibridge.c $host:spi/
ssh $host 'cd spi && gcc -W -Wall pibridge.c -lpthread && sudo ./a.out'
