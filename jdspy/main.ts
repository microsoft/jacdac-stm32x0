import * as U from "./pxtutils"
import * as HF2 from "./hf2"
import * as jd from "./jd"
import * as jdpretty from "./jdpretty"

const dev_ids = {
    "119c5abca9fd6070": "JDM3.0-ACC-burned",
    "ffc91289c5dc5280": "JDM3.0-ACC",
    "766ccc5755a22eb4": "JDM3.0-LIGHT",
    "259ab02e98bc2752": "F840-0",
    "69a9eaeb1a7d2bc0": "F840-1",
    "08514ae8a1995a00": "KITTEN-0",
    "XEOM": "DEMO-ACC-L",
    "OEHM": "DEMO-ACC-M",
    "MTYV": "DEMO-LIGHT",
    "ZYQT": "DEMO-MONO",
    "XMMW": "MB-BLUE",
    "CJFN": "DEMO-CPB",
}

U.jsonCopyFrom(jd.deviceNames, dev_ids)

async function main() {
    const startTime = Date.now()
    const hf2 = new HF2.Proto(new HF2.Transport())
    try {
        await hf2.init()

        hf2.onJDMessage(buf => {
            for (let p of jd.Packet.fromFrame(buf, Date.now() - startTime)) {
                jd.process(p)
                console.log(jdpretty.printPkt(p))
            }
        })

        jd.setSendPacketFn(p => {
            hf2.sendJDMessageAsync(p.toBuffer())
                .then(() => { }, err => console.log(err))
        })
    } catch (err) {
        console.error(err)
        await hf2.io.disconnectAsync()
    }
}

main()

