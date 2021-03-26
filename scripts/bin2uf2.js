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

// We first flash the bootBlock with app_reset_handler set to 0, then at the end of the file
// we flash it with the correct value.
// This is so that bootloader will detect failed flash and don't try to
// boot damaged application.
let bootBlockSize = 1024
const extTags = fetchExtensionTags()
const app_reset_offset = 13 * 4

const flags =
    0x00002000 | // familyID present
    0x00008000  // ext-tags present
const len = (buf.length + bootBlockSize) & ~(bootBlockSize - 1)
const numBlocks = (bootBlockSize + len) >> 8
const outp = []



function fetchExtensionTags() {
    let tags = {}
    try {
        tags = JSON.parse(fs.readFileSync(fn.replace(".bin", ".elf.json"), "utf8"))
    } catch {
        return Buffer.alloc(0)
    }
    if (tags["0x0be9f7"])
        bootBlockSize = tags["0x0be9f7"]
    const tagBuffers = []
    for (const ks of Object.keys(tags)) {
        const k = parseInt(ks)
        const v = tags[ks]
        let dat = null
        if (!k) throw "invalid tag"
        if (typeof v == "number") {
            dat = Buffer.alloc(4)
            dat.writeUInt32LE(v)
        } else if (typeof v == "string") {
            dat = Buffer.from(v, "utf8")
        } else {
            throw "invalid tag value"
        }
        const desig = Buffer.alloc(4)
        desig.writeUInt32LE(((k << 8) | (4 + dat.length)) >>> 0)
        tagBuffers.push(desig, dat)
        const tail = dat.length & 3
        if (tail)
            tagBuffers.push(Buffer.alloc(4 - tail))
    }
    return Buffer.concat(tagBuffers)
}


function addBlock(pos, buf) {
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
        bl[i + 32] = buf ? (buf[pos + i] || 0x00) : 0xff
    bl.set(extTags, 32 + 256)
    bl.writeUInt32LE(UF2_MAGIC_END, 512 - 4)
    outp.push(bl)

}


const app_reset = buf.readUInt32LE(app_reset_offset)
buf.writeUInt32LE(0, app_reset_offset)
for (let pos = 0; pos < len; pos += 256) {
    addBlock(pos, buf)
}
buf.writeUInt32LE(app_reset, app_reset_offset)
for (let pos = 0; pos < bootBlockSize; pos += 256) {
    addBlock(pos, buf)
}

if (numBlocks != outp.length) throw "oops";

const outfn = basename + ".uf2"
fs.writeFileSync(outfn, Buffer.concat(outp))
console.log(`Wrote ${numBlocks} blocks to ${outfn}`)
