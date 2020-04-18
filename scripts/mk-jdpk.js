const fs = require("fs")
let verbose = 1
if (process.argv[2] == "-q") {
    process.argv.shift()
    verbose = 0
}
const fn = process.argv[2]
const buf = fs.readFileSync(fn)

function log(msg) {
    if (verbose)
        console.log(msg)
}

let w0 = buf.readUInt32LE(0)
if ((w0 & 0xff00_0000) == 0x2000_0000) {
    const flashBase = 0x800_0000

    const basename = fn.replace(/\.bin$/, "")

    const dev_class = buf.readUInt32LE(0x20)

    if (dev_class >> 28 != 3)
        throw "invalid dev_class: " + dev_class.toString(16)

    const pageSz = 1024
    const pack = Buffer.alloc((buf.length + pageSz + pageSz - 1) & ~(pageSz - 1))
    const hddata = [
        0x4b50444a, // JDPK - magic
        0x1688f310, // almost random magic
        pageSz,
        flashBase,
        buf.length,
        dev_class,
    ]
    while (hddata.length < 16) hddata.push(0)

    for (let i = 0; i < hddata.length; ++i)
        pack.writeUInt32LE(hddata[i], i << 2)

    const desc = basename.replace(/^built\//, "").replace(/\/app-/, "/")
    pack.set(Buffer.from(desc), hddata.length * 4)
    pack.set(buf, pageSz)
    fs.writeFileSync(basename + ".jdpk", pack)
} else if (w0 == 0x9fddf13b) {
    log("skip bl file")
} else {
    throw "can't detect file type"
}

