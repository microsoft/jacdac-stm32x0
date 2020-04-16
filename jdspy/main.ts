import * as U from "./pxtutils"
import * as HF2 from "./hf2"

async function main() {
    const hf2 = new HF2.Proto(new HF2.Transport())
    try {
        await hf2.init()

        const t0 = Date.now()
        hf2.onJDMessage(buf => {
            console.log(`JD ${Date.now() - t0} ${U.toHex(buf)}`)
        })
    } catch (err) {
        console.error(err)
        await hf2.io.disconnectAsync()
    }
}

main()

