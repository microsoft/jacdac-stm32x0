let fs = require("fs")
let sums = { TOTAL: 0, "RAM.TOTAL": 0 }
let inprog = false
let inram = false
for (let line of fs.readFileSync(process.argv[2], "utf8").split(/\r?\n/)) {
  if (/\*fill\*/.test(line)) continue
  if (/^r[oa]m\s/.test(line)) continue
  let m = /^ \.(\w+)/.exec(line)
  if (m) {
    if (m[1] == "text" || m[1] == "binmeta" || m[1] == "rodata" || m[1] == "data")
      inprog = true
    else
      inprog = false
    if (m[1] == "data" || m[1] == "bss")
      inram = true
    else
      inram = false
  }
  if (!inprog && !inram) continue
  m = /\s+(0x00[a-f0-9]+)\s+(0x[a-f0-9]+)\s+(.*)/.exec(line)
  if (!m) continue
  let addr = parseInt(m[1])
  let size = parseInt(m[2])
  if (!addr || !size) continue
  let name = m[3]
  if (/load address/.test(name)) continue
  name = name.replace(/.*\/lib/, "lib")
  //    .replace(/\(.*/, "") // can remove

  let pref = inram ? "RAM." : ""

  // if (inram&&size > 500) console.log(line)

  name = pref + name

  //console.log(name, size, line)

  if (!sums[name]) sums[name] = 0
  sums[name] += size
  sums[pref + "TOTAL"] += size
}

function order(isram) {
  const k = Object.keys(sums).filter(s => /RAM/.test(s) == isram)
  k.sort((a, b) => sums[a] - sums[b])
  return k
}

for (let k of order(true).concat(order(false))) {
  console.log(k, sums[k])
}

