# SPI<->Jacdac bridge; RPi side

The executable in this folder talks via SPI to a STM32G0 MCU that handles real-time Jacdac communications.
The packets are read from stdin and then printed out to stdout, so it is easy to interface Python or Node.js to it.


Refer to [schematics.pdf](./schematics.pdf) file in this folder about how to build the hardware bridge.
Note that you'll likely want to use a different current limiter, with 0.9A limit or less.
You may also need additional ESD protection, please refer to [Jacdac module reference designs](https://github.com/microsoft/jacdac-ddk/tree/main/electronics/altium/reference-designs)

This is a prototype and has not been EMI-tested.

## Installation

Run `make` in this folder.
Put `pibridge` executable somewhere.

Enable SPI from `sudo raspi-config`.

When you run `./pibridge`, it should print some packets in hex.

Create (or update) `~/.jacdac/config.ini` with the following:

```ini
[jacdac]
transport_cmd = /path/to/pibridge
```

## RPi Zero USB

https://www.thepolyglotdeveloper.com/2016/06/connect-raspberry-pi-zero-usb-cable-ssh/
https://www.thepolyglotdeveloper.com/2019/07/share-internet-between-macos-raspberry-pi-zero-over-usb/

## Install node.js

(this is if you're not using our Python SDK)

```bash
wget https://nodejs.org/download/release/v10.24.1/node-v10.24.1-linux-armv6l.tar.xz
tar -xf node-v10.24.1-linux-armv6l.tar.xz
cd node-v10.24.1-linux-armv6l
sudo cp -r bin include lib share /usr/local
```

node from nodejs package from raspbian takes ~16s to startup (!!!)

install:
* git
