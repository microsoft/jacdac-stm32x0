const fs = require("fs")
const path = require("path")
const crypto = require("crypto")

let verbose = 1
if (process.argv[2] == "-q") {
    process.argv.shift()
    verbose = 0
}

const profiles_path = process.argv[2]

function error(msg) {
    console.error(msg)
    process.exit(1)
}

if (!fs.existsSync(profiles_path)) {
    error("USAGE: node check-fw-id.js <targets-folder>")
}

function log(msg) {
    if (verbose)
        console.log(msg)
}

const fwById = {}
const fwByName = {}

for (const dn of fs.readdirSync(profiles_path)) {
    if (dn[0] == "_")
        continue
    const prof_folder = path.join(profiles_path, dn, "profile")
    for (const fn of fs.readdirSync(prof_folder)) {
        if (!fn.endsWith(".c"))
            continue
        const profile_fn = path.join(prof_folder, fn)
        const src = fs.readFileSync(profile_fn, "utf8")

        const m = /FIRMWARE_IDENTIFIER\(((0x)?[0-9a-f]+),\s*"([^"]+)"\)/.exec(src)
        if (!m)
            error("FIRMWARE_IDENTIFIER(0x..., \"...\") missing")
        let dev_class = parseInt(m[1])
        const dev_class_name = m[3]
        if (!dev_class) {
            dev_class = ((crypto.randomBytes(4).readUInt32LE() >>> 4) | 0x30000000) >>> 0
            const trg = "0x" + dev_class.toString(16)
            let src2 = src.replace(m[0], `FIRMWARE_IDENTIFIER(${trg}, "${dev_class_name}")`)
            if (src == src2) throw "whoops"
            src2 = src2.replace("// The 0x0 will be replaced with a unique identifier the first time you run make.\n", "")
            fs.writeFileSync(profile_fn, src2)
            console.log(`Patching ${profile_fn}: dev_class ${m[1]} -> ${trg}`)
        }

        // log(`${profile_fn} -> 0x${dev_class.toString(16)} "${dev_class_name}"`)

        if ((dev_class >> 28) != 3)
            error("invalid FIRMWARE_IDENTIFIER() format")
        if (fwById[dev_class]) {
            error(`${fwById[dev_class]} and ${profile_fn} both use 0x${dev_class.toString(16)} as firmware identifier\n` +
                `Replace it with "0" in the newer of the two files and run "make" again.`)
        }
        if (fwByName[dev_class_name]) {
            error(`${fwById[dev_class]} and ${profile_fn} both use "${dev_class_name}"\n` +
                `Rename it in the newer of the two files and run "make" again.`)
        }

        fwById[dev_class] = profile_fn
        fwByName[dev_class_name] = profile_fn
    }
}
