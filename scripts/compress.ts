const blockSize = 1024
const blockSkip = 2
const reflen = 2
const bucketsBits = 12
const minsegsize = 4

const segmentSizes: number[] = []
for (let sz = blockSize; sz >= minsegsize; sz >>= 1)
    segmentSizes.push(sz)
const mingain = -10

let inputfile: Uint8Array
let buckets: Bucket[]

class Segment {
    hash: number
    gain: number
    allstarts: number[]

    constructor(public start: number, public length: number) {
        this.gain = -reflen
        this.hash = 0
        this.allstarts = null
    }

    init() {
        this.hash = fnv1a(inputfile, this.start, this.length)
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
        this.gain += this.length - reflen
        if (!this.allstarts) this.allstarts = [this.start]
        this.allstarts.push(other.start)
    }
}

function overlaps(s0: number, e0: number, s1: number, e1: number) {
    return (s0 <= s1 && s1 < e0) ||
        (s0 <= e1 && e1 < e0) ||
        (s1 <= s0 && s0 < e1) ||
        (s1 <= e0 && e0 < e1)
}

class Bucket {
    _maxgain: number = null
    segments: Segment[] = []

    get maxgain() {
        if (this._maxgain == null) {
            this._maxgain = mingain
            for (const s of this.segments) {
                this._maxgain = Math.max(s.gain, this._maxgain)
            }
        }
        return this._maxgain
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


function segmentsOverlapping(start: number, end: number) {
    const fileOff = start & ~(blockSize - 1)
    const res: Segment[] = []
    for (let blockOff = fileOff; blockOff < fileOff + blockSize; blockOff += blockSkip) {
        for (const segSize of segmentSizes) {
            if (blockOff + segSize <= fileOff + blockSize) {
                const s = new Segment(blockOff, segSize)
                if (s.isWithin(start, end) || s.contains(start, end)) {
                    s.init()
                    res.push(s)
                }
            }
        }
    }
    return res
}

function compressBlock(fileOff: number) {
    const res: Segment[] = []
    for (let blockOff = fileOff; blockOff < fileOff + blockSize; blockOff += blockSkip) {
        for (const segSize of segmentSizes) {
            if (blockOff + segSize <= fileOff + blockSize) {
                const s = new Segment(blockOff, segSize)
                if (s.isWithin(start, end) || s.contains(start, end)) {
                    s.init()
                    res.push(s)
                }
            }
        }
    }
    return res
}

function compress(_input: Uint8Array) {
    const t0 = Date.now()

    if (_input.length & (blockSize - 1))
        throw "oops"
    inputfile = _input
    buckets = new Array(1 << bucketsBits)

    for (let fileOff = 0; fileOff < inputfile.length; fileOff += blockSize) {
        for (const s of segmentsOverlapping(fileOff, fileOff + blockSize)) {
            const s2 = findSegment(s)
            if (s2) {
                s2.add(s)
            } else {
                const b = findBucket(s)
                b.segments.push(s)
            }
        }
    }

    let numsegs = 0
    let queue: Bucket[] = []
    for (const b of buckets) {
        if (b && b.maxgain > 0) {
            b.segments = b.segments.filter(s => s.gain > 0)
            queue.push(b)
            numsegs += b.segments.length
        }
    }

    const t1 = Date.now()
    console.log(`${numsegs} segments; ${queue.length} buckets; ${t1 - t0}ms`)

    const dictionary: Segment[] = []

    while (queue.length) {
        let maxbucket: Bucket = null
        queue = queue.filter(b => {
            if (b.maxgain <= 0) return false
            if (!maxbucket || b.maxgain > maxbucket.maxgain)
                maxbucket = b
            return true
        })
        if (!maxbucket) break
        let maxseg: Segment = null
        for (const s of maxbucket.segments) {
            if (!maxseg || maxseg.gain < s.gain)
                maxseg = s
        }

        dictionary.push(maxseg)
        for (const start of maxseg.allstarts || [maxseg.start]) {
            for (const s0 of segmentsOverlapping(start, start + maxseg.length)) {
                // this includes us
                const overlapSize = Math.min(maxseg.length, s0.length)
                const s = findSegment(s0)
                if (s) {
                    s.gain -= overlapSize
                    findBucket(s)._maxgain = null
                }
            }
        }
    }

    const t2 = Date.now()
    console.log(`${dictionary.length} entires in dictionary; ${t2 - t1}ms`)

    for (const b of buckets) {
        b.segments = []
    }
    for (const s of dictionary) {
        findBucket(s).segments.push(s)
    }


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
    compress(total)
}

declare var Buffer: any
declare var require: any
declare var process: any
const fs = require("fs")
concat(process.argv.slice(2).map((fn: string) => fs.readFileSync(fn)))
