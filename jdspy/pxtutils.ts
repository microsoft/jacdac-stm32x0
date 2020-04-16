export function assert(cond: boolean) {
    if (!cond) throw new Error("assertion failed")
}
export function delay<T>(millis: number, value?: T): Promise<T> {
    return new Promise((resolve) => setTimeout(() => resolve(value), millis))
}

export function memcpy(trg: Uint8Array, trgOff: number, src: ArrayLike<number>, srcOff?: number, len?: number) {
    if (srcOff === void 0)
        srcOff = 0
    if (len === void 0)
        len = src.length - srcOff
    for (let i = 0; i < len; ++i)
        trg[trgOff + i] = src[srcOff + i]
}

export function bufferToString(buf: ArrayLike<number>) {
    return Buffer.from(buf as any).toString("utf8")
}


export interface SMap<T> {
    [index: string]: T;
}

export class PromiseBuffer<T> {
    private waiting: ((v: (T | Error)) => void)[] = [];
    private available: (T | Error)[] = [];

    drain() {
        for (let f of this.waiting) {
            f(new Error("Promise Buffer Reset"))
        }
        this.waiting = []
        this.available = []
    }


    pushError(v: Error) {
        this.push(v as any)
    }

    push(v: T) {
        let f = this.waiting.shift()
        if (f) f(v)
        else this.available.push(v)
    }

    shiftAsync(timeout = 0) {
        if (this.available.length > 0) {
            let v = this.available.shift()
            if (v instanceof Error)
                return Promise.reject<T>(v)
            else
                return Promise.resolve<T>(v)
        } else
            return new Promise<T>((resolve, reject) => {
                let f = (v: (T | Error)) => {
                    if (v instanceof Error) reject(v)
                    else resolve(v)
                }
                this.waiting.push(f)
                if (timeout > 0) {
                    delay(timeout)
                        .then(() => {
                            let idx = this.waiting.indexOf(f)
                            if (idx >= 0) {
                                this.waiting.splice(idx, 1)
                                reject(new Error("Timeout"))
                            }
                        })
                }
            })
    }
}


export class PromiseQueue {
    promises: SMap<(() => Promise<any>)[]> = {};

    enqueue<T>(id: string, f: () => Promise<T>): Promise<T> {
        return new Promise<T>((resolve, reject) => {
            let arr = this.promises[id]
            if (!arr) {
                arr = this.promises[id] = []
            }
            const cleanup = () => {
                arr.shift()
                if (arr.length == 0)
                    delete this.promises[id]
                else
                    arr[0]()
            }
            arr.push(() =>
                f().then(v => {
                    cleanup()
                    resolve(v)
                }, err => {
                    cleanup()
                    reject(err)
                }))
            if (arr.length == 1)
                arr[0]()
        })
    }
}

export function toHex(bytes: ArrayLike<number>) {
    let r = ""
    for (let i = 0; i < bytes.length; ++i)
        r += ("0" + bytes[i].toString(16)).slice(-2)
    return r
}

export function fromHex(hex: string) {
    let r = new Uint8Array(hex.length >> 1)
    for (let i = 0; i < hex.length; i += 2)
        r[i >> 1] = parseInt(hex.slice(i, i + 2), 16)
    return r
}
