const fs = require("fs")

const b0 = fs.readFileSync(process.argv[2])
const b1 = fs.readFileSync(process.argv[3])

const len = Math.min(b0.length, b1.length)

if (b0.length != b1.length) {
    console.log(`only comparing first ${len} bytes`)
}

let numbytes = 0
let numbits = 0

for (let i = 0; i < len; ++i) {
    const a = b0[i]
    const b = b1[i]
    if (a == b)
        continue
    numbytes++
    let mask = 0x80;
    while (mask) {
        if ((a & mask) != (b & mask)) numbits++;
        mask >>= 1
    }
}

console.log(`${numbytes} bytes or ${numbits} bits different`)

