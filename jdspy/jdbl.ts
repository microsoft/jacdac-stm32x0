import * as U from "./pxtutils"
import * as HF2 from "./hf2"
import * as jd from "./jd"
import * as jdpretty from "./jdpretty"

const SERVCE_CLASS_BOOTLOADER = 0x1ffa9948

const BL_CMD_PAGE_DATA = 0x80
const BL_SUBPAGE_SIZE = 208


export async function flash(hf2: HF2.Proto, binProgram: Uint8Array) {
    const stackBase = U.read32(binProgram, 0)
    if ((stackBase & 0xff00_0000) != 0x2000_0000)
        throw "not a .bin file"

    const startTime = Date.now()
    let targetDevice: jd.Device
    let currPageAddr = -1
    let currPageError = -1

    let pageSize = 0
    let flashSize = 0
    let hwType = 0

    function timestamp() {
        return Date.now() - startTime
    }

    function log(msg: string) {
        console.log(`BL [${timestamp()}ms]: ${msg}`)
    }

    hf2.onJDMessage(buf => {
        for (let p of jd.Packet.fromFrame(buf, timestamp())) {
            jd.process(p)
            if (!targetDevice &&
                p.is_report &&
                p.service_number == 1 &&
                p.service_command == jd.CMD_ADVERTISEMENT_DATA) {
                const d = U.decodeU32LE(p.data)
                if (d[0] == SERVCE_CLASS_BOOTLOADER) {
                    pageSize = d[1]
                    flashSize = d[2]
                    hwType = d[3] // TODO match on this
                    targetDevice = p.dev
                    return
                }
            }

            if (targetDevice && p.dev == targetDevice && p.is_report && p.service_number == 1) {
                if (p.service_command == BL_CMD_PAGE_DATA) {
                    currPageError = U.read32(p.data, 0)
                    currPageAddr = U.read32(p.data, 4)
                    return
                }
            }

            const pp = jdpretty.printPkt(p, {
                skipRepeatedAnnounce: true,
                skipRepeatedReading: true
            })
            if (pp)
                console.log(pp)
        }
    })

    log("resetting all devices")

    const rst = jd.Packet.onlyHeader(jd.CMD_CTRL_RESET)
    await rst.sendAsMultiCommandAsync(0)

    log("asking for bootloaders")

    const p = jd.Packet.onlyHeader(jd.CMD_ADVERTISEMENT_DATA)
    while (!targetDevice && timestamp() < 15000) {
        await p.sendAsMultiCommandAsync(SERVCE_CLASS_BOOTLOADER)
        await U.delay(100)
    }

    if (!targetDevice)
        throw "timeout waiting for devices"

    if (binProgram.length > flashSize)
        throw "program too big"

    log(`flashing ${targetDevice}; available flash=${flashSize / 1024}kb; page=${pageSize}b`)
    const flashOff = 0x800_0000
    const hdSize = 7 * 4
    const numSubpage = ((pageSize + BL_SUBPAGE_SIZE - 1) / BL_SUBPAGE_SIZE) | 0

    for (let off = 0; off < binProgram.length; off += pageSize) {
        log(`flash ${off}/${binProgram.length}`)
        for (; ;) {
            let currSubpage = 0
            for (let suboff = 0; suboff < pageSize; suboff += BL_SUBPAGE_SIZE) {
                let sz = BL_SUBPAGE_SIZE
                if (suboff + sz > pageSize)
                    sz = pageSize - suboff
                const data = new Uint8Array(sz + hdSize)
                U.write32(data, 0, flashOff + off)
                U.write16(data, 4, suboff)
                data[6] = currSubpage++
                data[7] = numSubpage - 1
                data.set(binProgram.slice(off + suboff, off + suboff + sz), hdSize)
                const p = jd.Packet.from(BL_CMD_PAGE_DATA, data)
                p.service_number = 1
                await p.sendCmdAsync(targetDevice)
                await U.delay(5)
            }

            currPageError = -1
            for (let i = 0; i < 100; ++i) {
                if (currPageError >= 0)
                    break
                await U.delay(5)
            }
            if (currPageAddr != flashOff + off)
                currPageError = -2

            if (currPageError == 0) {
                break
            } else {
                log(`retry; err=${currPageError}`)
            }
        }
    }

    log("flash done; resetting target")

    const rst2 = jd.Packet.onlyHeader(jd.CMD_CTRL_RESET)
    await rst2.sendCmdAsync(targetDevice)

    await U.delay(300)
}
