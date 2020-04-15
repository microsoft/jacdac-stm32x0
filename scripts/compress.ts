const blockSizeBits = 10
const blockSize = 1 << blockSizeBits
const blockSkip = 2
const refantigain = 2
const bucketsBits = 12
const maxsegsize = 64
const minsegsize = 4
let startgain = -4

const segmentSizes: number[] = []
for (let sz = maxsegsize; sz >= minsegsize; sz >>= 1)
    segmentSizes.push(sz)
const mingain = -10

let inputfile: Uint8Array
let buckets: Bucket[]

class Segment {
    hash: number
    gain: number
    allstarts: number[]
    idx = -1

    constructor(public start: number, public length: number) {
        this.gain = startgain
        this.hash = 0
        this.allstarts = null
    }

    init() {
        this.hash = fnv1a(inputfile, this.start, this.length)
    }

    get score() {
        if (this.gain < 0) return this.gain
        return this.gain + (this.length << 20)
    }

    get bucketHash() {
        const h = this.hash
        return ((h ^ (h >>> bucketsBits)) & ((1 << bucketsBits) - 1)) >>> 0
    }

    get end() {
        return this.start + this.length
    }

    equals(other: Segment) {
        return this.hash == other.hash &&
            this.length == other.length &&
            sameMem(inputfile, this.start, other.start, this.length)
    }

    has(off: number) {
        return this.start <= off && off < this.end
    }

    isWithin(start: number, end: number) {
        return start <= this.start && this.end <= end
    }

    contains(start: number, end: number) {
        return this.start <= start && end <= this.end
    }

    overlaps(start: number, end: number) {
        return this.has(start) || this.has(end) ||
            (start <= this.start && this.start < end) ||
            (start <= this.end && this.end < end)
    }

    add(other: Segment) {
        if (other.start == this.start)
            throw "whoops"
        if (!other.equals(this))
            throw "whoops"
        this.gain += this.length - refantigain
        if (!this.allstarts) this.allstarts = [this.start]
        this.allstarts.push(other.start)
    }
}

function overlaps(s0: number, l0: number, s1: number, l1: number) {
    return overlapsSE(s0, s0 + l0, s1, s1 + l1)
}

function overlapsSE(s0: number, e0: number, s1: number, e1: number) {
    return (s0 <= s1 && s1 < e0) ||
        (s0 <= e1 && e1 < e0) ||
        (s1 <= s0 && s0 < e1) ||
        (s1 <= e0 && e0 < e1)
}

class Bucket {
    private _maxscore: Segment
    segments: Segment[] = []

    clearCache() {
        this._maxscore = undefined
    }

    get max() {
        if (this._maxscore === undefined) {
            this._maxscore = null
            let numdrops = 0
            for (const s of this.segments) {
                if (s.gain > 0) {
                    if (!this._maxscore || s.score > this._maxscore.score)
                        this._maxscore = s
                } else {
                    numdrops++
                }
            }
            if (numdrops)
                this.segments = this.segments.filter(s => s.gain > 0)
        }
        return this._maxscore
    }
}

function sameMem(buf: Uint8Array, s0: number, s1: number, len: number) {
    if (s0 == s1)
        return true
    for (let i = 0; i < len; ++i)
        if (buf[s0 + i] != buf[s1 + i])
            return false
    return true
}

function findBucket(s: Segment) {
    let b = buckets[s.bucketHash]
    if (!b) {
        b = buckets[s.bucketHash] = new Bucket()
    }
    return b
}

function findSegment(s: Segment) {
    const b = buckets[s.bucketHash]
    return b ? b.segments.find(ss => ss.equals(s)) : null
}

function fnv1a(data: Uint8Array, start: number, len: number) {
    let h = 0x811c9dc5
    for (let i = 0; i < len; ++i) {
        h = Math.imul(h ^ data[start + i], 0x1000193)
    }
    return h >>> 0
}


function segmentsAt(fileOff: number) {
    const res: Segment[] = []
    for (let blockOff = fileOff; blockOff < fileOff + blockSize; blockOff += blockSkip) {
        for (const segSize of segmentSizes) {
            const s = new Segment(blockOff, segSize)
            if (s.end <= fileOff + blockSize) {
                s.init()
                res.push(s)
            }
        }
    }
    return res
}

function compress(_input: Uint8Array) {
    const t0 = Date.now()

    if (_input.length & (blockSize - 1))
        throw "wrong input len"
    inputfile = _input
    buckets = new Array(1 << bucketsBits)
    buckets.fill(null)

    console.log(`input len: ${inputfile.length}`)

    for (let fileOff = 0; fileOff < inputfile.length; fileOff += blockSize) {
        for (const s of segmentsAt(fileOff)) {
            const s2 = findSegment(s)
            if (s2) {
                s2.add(s)
            } else {
                const b = findBucket(s)
                b.segments.push(s)
            }
        }
    }

    const segsAt: Segment[][] = new Array(inputfile.length >> blockSizeBits)
    for (let i = 0; i < segsAt.length; ++i)segsAt[i] = []

    let numsegs = 0
    let numreps = 0
    let queue: Bucket[] = []
    for (const b of buckets) {
        if (b && b.max && b.max.score > 0) {
            b.segments = b.segments.filter(s => {
                if (s.gain > 0) {
                    if (!s.allstarts) s.allstarts = [s.start]
                    for (const st of s.allstarts) {
                        segsAt[st >> blockSizeBits].push(s)
                        numreps++
                    }
                    return true
                }
                return false
            })
            queue.push(b)
            numsegs += b.segments.length
        }
    }

    const t1 = Date.now()
    console.log(`${numsegs} segments (${numreps} reps); ${queue.length} buckets; ${t1 - t0}ms`)

    const cover: Segment[] = new Array(inputfile.length / blockSkip)
    cover.fill(null)

    const dictionary: Segment[] = []

    while (queue.length) {
        let maxbucket: Bucket = null
        queue = queue.filter(b => {
            if (!b.max) return false
            if (!maxbucket || b.max.score > maxbucket.max.score)
                maxbucket = b
            return true
        })
        if (!maxbucket) break
        let maxseg = maxbucket.max
        maxseg.idx = dictionary.length
        dictionary.push(maxseg)

        for (const start of maxseg.allstarts) {
            if (start < 0) continue
            if (!sameMem(inputfile, maxseg.start, start, maxseg.length))
                throw "segment mem different"
            for (let i = start; i < start + maxseg.length; i += blockSkip) {
                if (cover[i / blockSkip])
                    throw ("already covered: " + i)
                cover[i / blockSkip] = maxseg
            }
            for (const other of segsAt[start >> blockSizeBits]) {
                let numcl = 0
                for (let i = 0; i < other.allstarts.length; ++i) {
                    const otherStart = other.allstarts[i]
                    if (otherStart < 0) continue
                    if (overlaps(start, maxseg.length, otherStart, other.length)) {
                        other.allstarts[i] = -1
                        other.gain -= other.length - refantigain
                        if (other.gain < -other.length + startgain) {
                            console.log(other)
                            throw "mingain"
                        }
                        numcl++
                    }
                }
                if (numcl)
                    findBucket(other).clearCache()
            }
        }
    }

    const t2 = Date.now()
    console.log(`${dictionary.length} entires in dictionary; ${cover.filter(c => c == null).length * 2} bytes unitary; ${t2 - t1}ms`)

    const outbytes: number[] = []

    function addbyte(n: number) {
        if (0 <= n && n <= 0xff) {
            outbytes.push(n)
        } else {
            throw "addbyte:" + n
        }
    }

    function addshort(n: number) {
        addbyte(n & 0xff)
        addbyte(n >> 8)
    }

    function addlong(n: number) {
        addbyte(n & 0xff)
        addbyte((n >> 8) & 0xff)
        addbyte((n >> 16) & 0xff)
        addbyte((n >> 24) & 0xff)
    }

    const posdiv = minsegsize
    addlong(0x706aef29)
    const szpos: any = {}
    for (const sz of segmentSizes) {
        addshort(sz)
        szpos[sz] = outbytes.length
        addshort(0) // patch later        
    }
    while (outbytes.length & (posdiv - 1))
        addbyte(0)
    for (const s of dictionary) {
        for (let i = 0; i < s.length; ++i)
            addbyte(inputfile[s.start + i])
        const p = outbytes.length / posdiv
        if ((p | 0) != p) throw "non-aligned: " + p
        s.idx = p
        if (p >= 0x8000) throw "out of range"
        outbytes[+szpos[s.length]] = p & 0xff
        outbytes[+szpos[s.length] + 1] = p >> 8
    }

    const numblocks: any = {}
    const blsize: any = {}

    function addhist(n: string, v: number) {
        if (!numblocks[n]) {
            numblocks[n] = 0
        }
        numblocks[n]++
        blsize[n] = v
    }

    for (let i = 0; i < cover.length;) {
        const s = cover[i]
        if (s) {
            i += s.length >> 1
            outbytes.push(s.idx & 0xff)
            outbytes.push(s.idx >> 8)
            addhist("seg:" + s.length, s.length)
        } else {
            const startbyte = i * 2
            let endbyte = Math.min(startbyte + 0x7f * 2, (startbyte & ~(blockSize - 1)) + blockSize)
            for (let pos = startbyte; pos < endbyte; pos += 2)
                if (cover[pos >> 1]) {
                    endbyte = pos
                    break
                }
            const halfs = (endbyte - startbyte) >> 1
            if (halfs > 0x7f)
                throw "unitary:" + halfs
            addbyte(halfs | 0x80)
            for (let pos = startbyte; pos < endbyte; pos++)
                addbyte(inputfile[pos])
            i += halfs
            addhist("bytes:" + (halfs * 2), halfs * 2)
        }
    }

    function szCover(key: string) {
        return numblocks[key] * blsize[key]
    }
    function szStore(key: string) {
        if (key[0] == "b")
            return (blsize[key] + 1) * numblocks[key]
        else
            return blsize[key] + 2 * numblocks[key]
    }

    const keys = Object.keys(numblocks)
    keys.sort((a, b) => szStore(a) - szStore(b))
    for (const key of keys) {
        console.log((szStore(key) * 100 / szCover(key)) | 0, szStore(key), szCover(key), numblocks[key], key)
    }

    console.log("TOTAL", outbytes.length)

    return new Uint8Array(outbytes)
}

function concat(inputs: Uint8Array[]) {
    let size = 0
    for (let inp of inputs) {
        size += (inp.length + blockSize - 1) & ~(blockSize - 1)
    }
    const total = new Uint8Array(size)
    size = 0
    for (let inp of inputs) {
        total.set(inp, size)
        size += (inp.length + blockSize - 1) & ~(blockSize - 1)
    }
    return compress(total)
}

declare var Buffer: any
declare var require: any
declare var process: any
const fs = require("fs")
const arr = concat(process.argv.slice(2).map((fn: string) => fs.readFileSync(fn)))
fs.writeFileSync("out.seg", Buffer.from(arr))

