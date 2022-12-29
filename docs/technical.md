# emonTx Technical Overview

The emonTx is an low power RF remote sensor node. Data is transmitted to an emonPi or an emonBase via a low power 433 MHz radio. This page provides a technical overview of the emonTx including configuration & firmware. For installation instructions see Hardware Setup guides.

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

## Power Supply Options

There are four ways to power the emonTx3:

1. USB to UART cable - only recommended for short periods while programming, it is recommended to remove all other power sources
2. 5V DC Mini-USB cable - remove jumper JP2 when powering via DC if AC adapter is present
3. 3 x AA Batteries - remove jumper JP2 when powering via DC if AC adapter is present
4. 9V AC-AC power adapter - with jumper JP2 closed (If jumper 2 is left open then the AC-AC adapter will be used for power sampling but not to power the emonTx3 - see AC adapter voltage sensing and power supply below).

## AC adapter voltage sensing and power supply.

The emonTx3 is designed to use an AC-AC adapter to provide an AC voltage sample and also depending on the configuration power as well. If the emonTx3 detects the presence of an AC adapter at startup, it will automatically implement Real Power and Vrms measurements by sampling the AC voltage. Real Power is what you get billed for, and depending on the appliances connected to the circuit being monitored, can vary significantly from Apparent Power - see Learn for more info on AC power theory. **For best energy monitoring accuracy, we recommend powering the emonTx3 with an AC-AC adapter whenever possible**.

Using the AC-AC adapter also enables the emonTx to monitor the direction of current flow. This is important for solar PV monitoring. If you notice a negative reading when you were expecting a positive one, reverse the orientation of the CT on the conductor.

Powering via the AC adapter is only suitable for standard CT monitoring and up to 4x DS18B20 temperature sensors, transmitting data via RFM69 433Mhz radio. The power supply circuit is only able to deliver a limited amount of current without impacting the voltage sensor accuracy.

To avoid damage to the emonTx3 circuits, the current drawn from the AC circuit should never exceed 60mA. If more than 10 mA of current is required, it is recommended to remove jumper 2 (JP2) and power the emonTx via the 5V mini-USB connector. When JP2 is removed, the AC-AC adapter (if connected) will be used only to provide an AC voltage sample, i.e. it will not power the emonTx.

**If using the emonTx with an ESP8266 WiFi adapter a seperate 5V USB power supply is required.**

## Schematic and Board files

Proudly open source, the hardware designs are released under the Creative Commons Attribution-ShareAlike 3.0 Unported License: 

**emonTx Schematic and Board files:**<br> [https://github.com/openenergymonitor/emontx3/tree/master/hardware](https://github.com/openenergymonitor/emontx3/tree/master/hardware)

**emonTx Wiki:**<br> [https://wiki.openenergymonitor.org/index.php/EmonTx_V3.4](https://wiki.openenergymonitor.org/index.php/EmonTx_V3.4)

## Port Map

![EmonTx_V3.4_portmap.png](img/EmonTx_V3.4_portmap.png)

![](img/EmonTx_V3.4_brd_values_white.png)

*Note: The FTDI connector Tx and Rx pins are reversed on the PCB legend and on the Schematic. Data is received by the emonTx on the Tx pin and transmitted by the emonTx on the Rx pin.*

## Datasheet table

| Function                   | Parameter                          | Min     | Recommended  Max / typical   | Absolute Max  if exceeded, damage may occur       | Notes                                                      |
|----------------------------|------------------------------------|---------|------------------------------|---------------------------------------------------|------------------------------------------------------------|
| CT 1-3                     | Monitoring Power @ 240V            |         | 23kW / 95.8A                 | 60kW / 250A                                       | Using 22R burden and  YHDC SCT-013-00                      |
| CT 4                       | Monitoring Power @ 240V            |         | 4.5kW / 19.2A                | 4.6kW max measure – 19.7kW / 82A Max dissipation  | Using 120R burden and  YHDC SCT-013-00                     |
| DC half-wave power supply  | Current output                     |         | 20mA                         | 60mA                                              | Current draw above 20mA AC sample signal will be affected  |
| 3.3V Rail current output   | When powered via 5V USB connector  |         | 150mA                        | 168mA                                             | Limitation SOT22 MCP1700 Ta=40C Vi=5.25V                   |
| Operating Temperature      | Temperature                        | -40C    | +40C                         | +85C                                              |                                                            |
| Storage Temperature        | Temperature                        | -50C    |                              | +150C                                             |                                                            |
| 5V Input Voltage           | USB/FTDI/5V Aux                    | +3.4V   | +6V                          | +6.5V (see note 1)                                | See note 1                                                 |
| 3.3V Supply Voltage        | on 3.3V supply rail                | 2.6V *  | 3.3V                         | 3.9V RFM69CW                                      | *ADC readings will be incorrect if Vcc is not 3.3V         |

*A higher voltage (up to 17.6V) can be used to power the emonTx if power is connected into the MCP1754 through the AC-DC enable jumper pin.*

## Enclosure

The emonTx V3 PCB is 100mm x 80mm and enclosed in an EBS80 enclosure sourced from Lincoln Binns, see [data sheet](https://www.lincolnbinns.com/en/aluminium-electronic-enclosure-e-case-b-data).

**Community contributed**

- [emonTx DIN-RAIL mount #1 fixing on Thingiverse](http://www.thingiverse.com/thing:1355749)
- [emonTx DIN-RAIL mount #2 fixing on Thingiverse](https://www.thingiverse.com/thing:208177)
- [emonTx > EmonESP > LCD](https://www.thingiverse.com/thing:2043784)
- [emonTx 3D printed case design](https://www.thingiverse.com/nduarte/designs)
