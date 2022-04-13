const fs = require("fs")
const path = require("path")
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
const profiles_path = process.argv[5]
const page_size = parseInt(process.argv[6])
const isBLU = process.argv[7] == "blup=1"

const DEV_INFO_MAGIC = 0xf6a0e4b6

if (isNaN(page_size) || !profiles_path) {
    throw "USAGE: node patch-bin.js file.elf flash_size_in_k bootloader_size_in_k profiles_path page_size"
}

let pos = 0

function log(msg) {
    if (verbose)
        console.log(msg)
}

let w0 = buf.readUInt32LE(0)
if (w0 == 0x464c457f) {
    const out = child_process.execSync("arm-none-eabi-objdump -h " + fn, { encoding: "utf-8" })
    const m = /^\s*\d+\s+\.text\s.*\s([0-9a-fA-F]{8})\s+\d\*\*/m.exec(out)
    if (!m) {
        throw ("invalid output: " + out)
    }
    pos = parseInt(m[1], 16)
    log("detected ELF file, text at " + pos.toString(16))
} else {
    log("assuming BIN file")
}

function fnv1a(data) {
    let h = 0x811c9dc5
    for (let i = 0; i < data.length; ++i) {
        h = Math.imul(h ^ data.charCodeAt(i), 0x1000193)
    }
    return h
}

w0 = buf.readUInt32LE(pos)
if ((w0 & 0xff000000) == 0x20000000) {
    log("app mode")

    const flashBase = 0x08000000

    const basename = fn.replace(/\.elf$/, "")

    if (profiles_path.indexOf("\\_") > 0 || profiles_path.indexOf("/_") > 0)
        throw "folder names cannot start with _"

    // figure out device class
    const profile_name = basename.replace(/.*\/(app|blup)-/, "")
    const profile_fn = profiles_path + "/" + profile_name + ".c"
    const src = fs.readFileSync(profile_fn, "utf8")
    const m = /FIRMWARE_IDENTIFIER\((0x3[0-9a-f]+),\s*"([^"]+)"\)/.exec(src)
    if (!m)
        throw "FIRMWARE_IDENTIFIER(0x3..., \"...\") missing"
    let dev_class = parseInt(m[1])
    let dev_class_name = m[2]
    if (isBLU)
        dev_class_name = "BOOTLOADER: " + dev_class_name
    log(`device class: 0x${dev_class.toString(16)} "${dev_class_name}"`)

    const reset = buf.readUInt32LE(pos + 4)
    const app_reset = buf.readInt32LE(pos + 13 * 4)
    if (app_reset == 0 || app_reset == -1) {
        buf.writeUInt32LE(reset, pos + 13 * 4)
        log("patching app_reset to " + reset.toString(16))
    }

    const bl_reset_handler = flashBase + (flash_size - bl_size) * 1024 + 8 * 4 + 1
    buf.writeUInt32LE(bl_reset_handler, pos + 4)
    log("setting global reset to " + bl_reset_handler.toString(16))

    buf.fill(0xff, pos + 7 * 4, pos + (7 + 4) * 4)
    buf.writeUInt32LE(dev_class, pos + 8 * 4)
    log("clearing devinfo area")

    const vm = /"([^"]+)"/.exec(fs.readFileSync(path.join(path.dirname(fn), "version.c"), "utf8"))
    const fw_version = vm[1]

    fs.writeFileSync(fn + ".json", JSON.stringify({
        "0xc8a729": dev_class,
        "0x0be9f7": page_size,
        "0x650d9d": dev_class_name,
        "0x9fc7bc": fw_version
    }))

} else if (w0 == DEV_INFO_MAGIC) {
    // no longer needed
    // log("setting random seed")
    // require("crypto").randomFillSync(buf, pos + 16, 8)
} else {
    throw "can't detect file type"
}


fs.writeFileSync(fn, buf)
