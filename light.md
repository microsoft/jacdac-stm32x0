# Light service protocol

Realistically, with 1 mbit JACDAC, we can transmit under 2k of data per animation frame (at 20fps).
If transmitting raw data that would be around 500 pixels, which is not enough for many
installation and it would completely clog the network.

Thus, light service defines a domain-specific language for describing light animations
and efficiently transmitting them over wire.

Light commands are not JACDAC commands.
Light commands are efficiently encoded as sequences of bytes and typically sent as payload
of `LIGHT_RUN` command.

## Supported commands

Definitions:
* `P` - position in the strip
* `R` - number of repetitions of the command
* `N` - number of pixels affected by the command
* `C` - single color designation
* `C+` - sequence of color designations

Update modes:
* `0` - replace
* `1` - add RGB
* `2` - subtract RGB
* `3` - multiply RGB (by c/128)

Commands:

* `0xD0: set_all(C+)` - set all pixels in current range to given color pattern
* `0xD1: fade(C+)` - set `N` pixels to color between colors in sequence
* `0xD2: fade_hsv(C+)` - similar to `fade()`, but colors are specified and faded in HSV
* `0xD3: rotate_fwd(K=1)` - rotate (shift) pixels by `K` positions away from the connector in given range
* `0xD4: rotate_back(K=1)` - same, but towards the connector
* `0xD5: show(M=50)` - send buffer to strip and wait `M` milliseconds
* `0xD6: range(P=0, N=length, W=1, S=0)` - range from pixel `P`, `N` pixels long
  (currently unsupported: every `W` pixels skip `S` pixels)
* `0xD7: mode(K=0)` - set update mode
* `0xD8: mode1(K=0)` - set update mode for next command only

* `0xD9: set(P=0, C+)` - set pixels at `P` in current range to color pattern (once)

The `P`, `R`, `N` and `K` can be omitted.
If only one of the two number is omitted, the remaining one is assumed to be `P`.
Default values:
```
R = 1
K = 1
N = length of strip
P = 0
M = 50
```

`set(P, N, C)` (with single color) is equivalent to `fade(P, N, C)` (or `fade_hsv()`).

## Command encoding

A number `k` is encoded as follows:
* `0 <= k < 128` -> `k`
* `128 <= k < 16383` -> `0x80 | (k >> 8), k & 0xff`
* bigger and negative numbers are not supported

Thus, bytes `0xC0-0xFF` are free to use for commands.

Formats:
* `0xC1, R, G, B` - single color parameter
* `0xC2, R0, G0, B0, R1, G1, B1` - two color parameter
* `0xC3, R0, G0, B0, R1, G1, B1, R2, G2, B2` - three color parameter
* `0xC0, N, R0, G0, B0, ..., R(N-1), G(N-1), B(N-1)` - `N` color parameter

Commands are encoded as command byte, followed by parameters in the order
from the command definition.

TODO:
* subranges - stateful - drop fade etc parameters?
* additive mode
* dithering?
* multiply all pixels by given intensity (hsv?)
