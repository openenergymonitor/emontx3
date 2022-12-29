# Overview

The emonTx3 is a low power RF remote sensor node. Data is transmitted to an emonPi or an emonBase via a low power 433 MHz radio.

![emonTx](img/emontx.jpg)

**Features:**

- 4x CT Sensor inputs for single-phase AC electricity monitoring designed for use with the [100A SCT-013-000 CT sensor](https://shop.openenergymonitor.com/100a-max-clip-on-current-sensor-ct/)
- CT 1-3 are standard 100A / 24KW max CT inputs (@240V)
- CT 4 is a special high sensitivity input for 19A / 4.6KW (@240V)
- 1x AC voltage measurement using plug-in AC-AC adapter for real power calculation alongside current measurement from CT sensors. Designed for use with a [9V AC output voltage adapter](https://shop.openenergymonitor.com/ac-ac-power-supply-adapter-ac-voltage-sensor-uk-plug/)
- Support for multiple wired one-wire DS18B20 temperature sensors via RJ45 socket or terminal block.
- Support for pulse counting either wired or via Optical Pulse Sensor
- Can be powered by a single AC-AC adaptor (DC PSU not required)
- Battery power option via 3 x AA batteries
- RF Range is approximately similar to home WiFi (real world range depends on many factors e.g. thick stone walls)
- Up to 2x emonTx can be connected to a single emonPi or emonBase (up to 30x is possible with manual RF node ID setting*)
- Wall-mount option
- New 2019: The emonTx firmware now supports higher accuracy continuous monitoring as standard, power from an ACAC adapter or USB power is assumed.
- Alternative firmware options include: Discrete Sampling firmware - useful when only battery power is available; and 3-phase firmware.

**WiFi option:** It is possible to use an emonTx with a ESP8266 WiFi adapter to provide WiFi connectivity. This can be useful in applications where the 433 MHz radio range is not sufficient but there is good WiFi signal. The emonTx can post data to a local emonBase/emonPi over WiFi or work in 'standalone mode' to post directly to a remote emoncms server such as emoncms.org. See [Using emonTx with the ESP8266 WiFi module](esp8266.md)

**Battery vs AC adapter:**
An emonTx can be powered by 3 x AA batteries; however, if possible, it is recommended to power the unit with an AC-AC adapter to provide an AC voltage reference for more accurate real power calculation and continuous sampling support.
