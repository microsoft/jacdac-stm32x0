#!/usr/bin/env node
"use strict";

const fs = require("fs")
const fn = process.argv[2]
if (!fn)
    throw "USAGE: node bin2uf2.js file.bin"


const buf0 = fs.readFileSync(fn)

let hex = "extern const unsigned char bootloader[];\n"
hex += "const unsigned char bootloader[] __attribute__((aligned(8))) = {\n"
const bufpref = Buffer.alloc(16)
bufpref.writeUInt32LE(0x2fd56055, 0)
bufpref.writeUInt32LE(buf0.length, 4)
const bufsuff = Buffer.alloc(16)
const buf = Buffer.concat([bufpref, buf0, bufsuff])
for (let i = 0; i < buf.length; ++i) {
    hex += "0x" + ("0" + buf[i].toString(16)).slice(-2) + ","
    if (i % 16 == 15) hex += "\n"
    else hex += " "
}
hex += "};\n"
fs.writeFileSync(fn.replace(".bin", ".c"), hex)
