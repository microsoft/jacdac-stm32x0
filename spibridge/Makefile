all:
	@pkg-config --exists libgpiod || echo "you may need to run: sudo apt-get install libgpiod-dev"
	gcc -W -Wall pibridge-gpiod.c -lgpiod -pthread -o pibridge

all-wiringpi:
	gcc -W -Wall pibridge-wiringpi.c -lwiringPi -pthread -o pibridge
