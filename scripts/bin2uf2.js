#!/usr/bin/env node
"use strict";

const UF2_MAGIC_START0 = 0x0A324655 // "UF2\n"
const UF2_MAGIC_START1 = 0x9E5D5157 // Randomly selected
const UF2_MAGIC_END = 0x0AB16F30   // Ditto

const fs = require("fs")
const fn = process.argv[2]
if (!fn)
    throw "USAGE: node bin2uf2.js file.bin"


const buf = fs.readFileSync(fn)
const startAddr = buf.readUInt32LE(8) & 0xff000000
const basename = fn.replace(/\.bin$/, "")

const dev_class = buf.readUInt32LE(0x20)
if (dev_class >> 28 != 3)
    throw "invalid dev_class: " + dev_class.toString(16)
const familyID = dev_class

const flags = 0x00002000 // familyID present
const numBlocks = (buf.length + 255) >> 8
const outp = []
for (let pos = 0; pos < buf.length; pos += 256) {
    const bl = Buffer.alloc(512)
    bl.writeUInt32LE(UF2_MAGIC_START0, 0)
    bl.writeUInt32LE(UF2_MAGIC_START1, 4)
    bl.writeUInt32LE(flags, 8)
    bl.writeUInt32LE(startAddr + pos, 12)
    bl.writeUInt32LE(256, 16)
    bl.writeUInt32LE(outp.length, 20)
    bl.writeUInt32LE(numBlocks, 24)
    bl.writeUInt32LE(familyID, 28) // reserved
    for (let i = 0; i < 256; ++i)
        bl[i + 32] = buf[pos + i]
    bl.writeUInt32LE(UF2_MAGIC_END, 512 - 4)
    outp.push(bl)
}

if (numBlocks != outp.length) throw "oops";

const outfn = basename + ".uf2"
fs.writeFileSync(outfn, Buffer.concat(outp))
console.log(`Wrote ${numBlocks} blocks to ${outfn}`)
