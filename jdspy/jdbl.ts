import * as U from "./pxtutils"
import * as HF2 from "./hf2"
import * as jd from "./jd"
import * as jdpretty from "./jdpretty"

/*

#define BL_PAGE_SIZE FLASH_PAGE_SIZE
#define BL_NUM_SUBPAGES ((BL_PAGE_SIZE + SUBPAGE_SIZE - 1) / SUBPAGE_SIZE)

struct bl_page_data {
    uint32_t pageaddr;
    uint16_t pageoffset;
    uint8_t subpageno;
    uint8_t subpagemax;    
    uint32_t reserved[5];
    uint8_t data[BL_SUBPAGE_SIZE];
};



*/

const SERVCE_CLASS_BOOTLOADER =  0x1ffa9948

const BL_CMD_PAGE_DATA = 0x80
const BL_SUBPAGE_SIZE = 208

export async function flash(hf2: HF2.Proto, buf: Uint8Array) {
    const stackBase = U.read32(buf, 0)
    if ((stackBase & 0xff00_0000) != 0x2000_0000)
        throw "not a .bin file"

    const startTime = Date.now()
    hf2.onJDMessage(buf => {
        for (let p of jd.Packet.fromFrame(buf, Date.now() - startTime)) {
            jd.process(p)
            const pp = jdpretty.printPkt(p, {
                skipRepeatedAnnounce: true,
                skipRepeatedReading: true
            })
            if (pp)
                console.log(pp)
        }
    })

    const p = jd.Packet.onlyHeader(jd.CMD_ADVERTISEMENT_DATA)
    p.sendAsMultiCommand(SERVCE_CLASS_BOOTLOADER)
    p.sendAsMultiCommand(SERVCE_CLASS_BOOTLOADER)
}