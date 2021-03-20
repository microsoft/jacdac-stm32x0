const fs = require("fs")
const path = require("path")
const child_process = require("child_process")

let markdown = 0

async function logfor(dir, myurl, range) {
    if (!range) range = ""

    const submoduleStart = {}
    const submoduleEnd = {}

    const submoduleUrl = {}

    try {
        const mods = fs.readFileSync(".gitmodules", "utf8")
        mods.replace(/\[submodule "([^"]+)"\][^\[]*\surl = (.*)/g, (_, sm, url) => {
            submoduleUrl[sm] = url
            return ""
        })
    } catch { }

    if (!myurl) {
        const confi = fs.readFileSync(".git/config", "utf8")
        const m = /\[remote "origin"\][^\[]*\surl = (.*)/.exec(confi)
        myurl = m[1]
    }

    let res = `## Changelog for ${myurl.replace(/.*github.com\//, "")}\n\n`

    await new Promise((resolve, reject) => child_process.exec(`git log --submodule=log --oneline --patch --decorate=full ${range}`, {
        maxBuffer: 100 * 1024 * 1024,
        windowsHide: true,
        cwd: dir
    }, (err, stdout, stderr) => {
        if (err)
            return reject(err)
        if (stderr)
            return reject(new Error("stderr: " + stderr))
        let numcommits = 0
        for (const line of stdout.split(/\r?\n/)) {
            let m = /^([0-9a-f]{7}) (.*)/.exec(line)
            if (m) {
                const commit = m[1]
                let msg = m[2]
                let tags = ""
                m = /^(\([^\)]+\)) (.*)/.exec(msg)
                if (m && /refs\//.test(m[1])) {
                    tags = m[1]
                    msg = m[2]
                }

                // found previous tag?
                if (!range && numcommits && /refs\/tags\/v[0-9]/.test(tags))
                    break

                // note that /pull/123 will redirect to /issue/123 if it's an issue
                if (markdown)
                    msg = msg.replace(/#(\d+)/g, (f, n) => `[${f}](${myurl}/pull/${n})`)

                numcommits++
                if (markdown)
                    res += `* [${commit}](${myurl}/commit/${commit}) ${msg}\n`
                else
                    res += `${commit} ${msg}\n`
            }
            m = /^Submodule (\S+) ([0-9a-f]+)\.\.([0-9a-f]+)/.exec(line)
            if (m) {
                const sm = m[1]
                if (!submoduleEnd[sm])
                    submoduleEnd[sm] = m[3]
                submoduleStart[sm] = m[2]
            }
        }
        resolve()
    }))

    const subs = Object.keys(submoduleStart)
    subs.sort()

    for (const sm of subs) {
        res += "\n"
        res += await logfor(path.join(dir, sm), submoduleUrl[sm], submoduleStart[sm] + ".." + submoduleEnd[sm])
    }

    return res
}

async function main() {
    try {
        const res = await logfor(".")
        console.log(res.trim())
    } catch (e) {
        console.error(e)
    }
}

main()