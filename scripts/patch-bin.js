const fs = require("fs")
const child_process = require("child_process")
let verbose = 1
if (process.argv[2] == "-q") {
    process.argv.shift()
    verbose = 0
}
const fn = process.argv[2]
const buf = fs.readFileSync(fn)

const flash_size = parseInt(process.argv[3])
const bl_size = parseInt(process.argv[4])

if (isNaN(bl_size)) {
    throw "USAGE: node patch-bin.js file.elf flash_size_in_k bootloader_size_in_k"
}

let pos = 0

function log(msg) {
    if (verbose)
        console.log(msg)
}

const w0 = buf.readUInt32LE(0)
if (w0 == 0x464c457f) {
    const out = child_process.execSync("arm-none-eabi-objdump -h " + fn, { encoding: "utf-8" })
    const m = /^\s*\d+\s+\.text\s.*\s([0-9a-fA-F]{8})\s+\d\*\*/m.exec(out)
    if (!m) {
        throw ("invalid output: " + out)
    }
    pos = parseInt(m[1], 16)
    log("detected ELF file, text at " + pos.toString(16))
} else if ((w0 & 0xff00_0000) == 0x2000_0000) {
    log("detected BIN file")
}

const reset = buf.readUInt32LE(pos + 4)
const app_reset = buf.readInt32LE(pos + 13 * 4)
if (app_reset == 0 || app_reset == -1) {
    buf.writeUInt32LE(reset, pos + 13 * 4)
    log("patching app_reset to " + reset.toString(16))
}

const bl_reset_handler = 0x800_0000 + (flash_size - bl_size) * 1024 + 4 * 4 + 1
buf.writeUInt32LE(bl_reset_handler, pos + 4)
log("setting global reset to " + bl_reset_handler.toString(16))

buf.fill(0xff, pos + 7 * 4, pos + (7 + 4) * 4)
log("clearing devinfo area")

fs.writeFileSync(fn, buf)
