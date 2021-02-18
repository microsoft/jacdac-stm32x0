# Jacdac power

JD device that delivers power to the bus needs to limit it.
This document has notes on one way of implementing this.

In both options:
* add FET high-side switch on power delivery to the bus
* probe current every 1ms, with slight randomization (eg. every 900-1100us)

## Cheap option

* add shunt resistor on the low side of power line (GND)
* connect bus side of shunt to ADC via 3/5 voltage divider

Assuming 12-bit ADC with reference voltage of 3.3V the table
below gives precision of measurement per ADC step and voltage
drop at 500mA.

Shunt | Vdrop | Precision | Size
-----:|------:|----------:|-----
0.50Ω |  5.0% |     2.7mA | 0603
0.36Ω |  3.6% |     3.7mA | 0402
0.22Ω |  2.2% |     6.1mA | 0402
0.20Ω |  2.0% |     6.7mA | 0402

## Precise option

* add INA199A1 current sensing amplifier (50V/V)
* add shunt on either power or GND side

Shunt | Vdrop | Precision | Max Curr | Size
-----:|------:|----------:|---------:|------
0.10Ω |  1.0% |   0.161mA |    660mA | 0603
0.05Ω |  0.5% |   0.322mA |   1320mA | 0805
0.02Ω |  0.2% |   0.806mA |   3300mA | 0805

Additionally, can add additional switching FET and another shunt
for precision measurements, when current is low.
Voltage drops listed for 50mA.

Shunt | Vdrop | Precision | Max Curr | Size
-----:|------:|----------:|---------:|------
1.00Ω |  1.0% |   0.016mA |     66mA | 0402
0.50Ω |  0.5% |   0.032mA |    132mA | 0603

## Low pass filters

With 100Hz RC filter, if the current goes from max to 0,
the voltage will drop by ~45% in 1ms. This represents
a measure of error (the current we may miss when not sampling).
OTOH, it takes 2ms for voltage to reach 70% when it goes from 0 to max,
which is a measure of latency.

With 50Hz, it drops by ~25% in 1ms and it takes 4ms to register
70% of rise.

With 25Hz, it drops by ~15% in 1ms and it takes 8ms to register
70% of rise.

50Hz is eg. 3.3kΩ and 1uF.

Data from http://sim.okawa-denshi.jp/en/PWMtool.php

