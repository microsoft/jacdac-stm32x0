/*
Compute pairs of resistor and capacitor values that can be distinguished by STM32G030 (or similar).
It identifies 49 different pairs. Each pair can be connected up or down, for 98 distinct values.

External pull-down:
MCU ----+----+
        |    |
        R    C
        |    |
       GND  GND

External pull-up:
       VCC  VCC
        |    |
        R    C
        |    |
MCU ----+----+

Example output for resistors:

0.012V - 0.026V  -- 200 Ohm
0.040V - 0.088V  -- 680 Ohm
0.115V - 0.247V  -- 2000 Ohm
0.257V - 0.527V  -- 4700 Ohm
0.586V - 1.077V  -- 12000 Ohm
1.230V - 1.886V  -- 33000 Ohm
1.896V - 2.481V  -- 75000 Ohm
2.583V - 2.937V  -- 200000 Ohm
2.951V - 3.135V  -- 470000 Ohm

To identify the pair:
1. enable pull-up, wait 100ms, measure voltage
   if voltage is 3.3V, then we have external pull-down, proceed to step 2
   otherwise, based on voltage determine value of R and go to step 3
2. enable pull-down, wait 100ms, measure voltage
   determine value of R, go to step 3
3. disable pull-up/down
   energize the capacitor by setting line to opposite of external pull
   switch line to input
   measure time until line changes state
   determine C based on measured R and the table from this script

*/

// in nF, 10%
const caps = [0.033, 0.047, 0.068, 0.100, 0.150, 0.220, 0.470, 1, 2.2, 4.7, 6.8, 10, 22, 47, 100, 220, 470, 1000]

// in Ohm, 1%
const resistors = [
    0, 1, 10, 22, 33, 47, 51, 75, 100, 120, 150, 200, 220, 300, 330, 470, 499, 510, 680,
    1000, 1200, 1500, 2000, 2200, 2400, 3300, 3900, 4700, 5100, 5600, 6800, 7500, 8200,
    10000, 12000, 15000, 18000, 20000, 22000, 24000, 27000, 33000, 39000, 47000, 49900, 51000, 56000, 68000, 75000,
    100000, 120000, 150000, 200000, 220000, 300000, 330000, 470000, 510000,
    1000000, 10000000]

const pullups = [25000, 40000, 55000]

// V
const V_supply = 3.3
const V_gray = [1.227, 1.877]

const adc_jitter = 0.010
const timer_jitter = 5

function cmp(a, b) { return a - b }

function divider(r1) {
    const results = []
    for (const r2 of pullups) {
        let r = r1 * 1.01
        results.push(V_supply * r / (r + r2))
        r = r1 * 0.99
        results.push(V_supply * r / (r + r2))
    }
    results.sort(cmp)
    return [results[0], results[results.length - 1]]
}

function listResistors() {
    let maxV = adc_jitter
    const rr = []
    for (const r of resistors) {
        const [min, max] = divider(r)
        if (min > maxV && max < V_supply - adc_jitter) {
            console.log(`${min.toFixed(3)}V - ${max.toFixed(3)}V  -- ${r} Ohm`)
            maxV = max + adc_jitter
            rr.push(r)
        }
    }
    return rr
}

function dischargeTime(C0, R0) {
    const results = []
    C0 /= 1000 // we have nF not F, but we want us not s
    for (const C of [C0 * 0.9, C0 * 1.1]) {
        for (const R of [R0 * 0.99, R0 * 1.01]) {
            for (const V of V_gray) {
                const t = -C * R * Math.log(V / V_supply)
                results.push(t)
            }
        }
    }
    results.sort(cmp)
    return [results[0] | 0, results[results.length - 1] | 0]
}

let outp = 0

function capsForR(R) {
    let maxT = timer_jitter
    const cc = []
    for (const C of caps) {
        const [min, max] = dischargeTime(C, R)
        if (max > 300000)
            break
        if (min > maxT) {
            cc.push(C)
            console.log(`${++outp}. ${R} Ohm ${min}us - ${max}us -> ${C} nF`)
            maxT = max + timer_jitter
        }
    }
    return cc
}

for (const R of listResistors())
    capsForR(R)
