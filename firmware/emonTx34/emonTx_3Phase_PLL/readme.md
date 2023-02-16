# emonTx_3Phase_PLL

This firmware is a development of the 3-phase ‘discrete sample’ sketch and MartinR’s PLL
energy diverter.

It is intended for use on a **3-phase, 4-wire system**. It utilises advanced features of the Atmel328P microprocessor to provide continuous monitoring of voltage of one phase and the currents in all three phases, thereby allowing a good estimate of real power to be calculated. The physical quantities are measured continuously, the average values are calculated and transmitted at user-defined intervals.

Pulse counting and temperature monitoring using a single DS18B20 sensor is supported.

*By [Robert Wall](https://community.openenergymonitor.org/u/robert.wall/summary)*

## Key Feautures

- Continuous monitoring of one voltage channel and up 4 current channels.
- Gives an accurate measure of rapidly varying loads.
- 1800 sample sets per second – using 4 channels of an emonTx V3.4 @ 50 Hz
- Calculates rms voltage, rms current, real & apparent power & power factor.
- Pulse input for supply meter monitoring.
- Integrated temperature measurement (one DS18B20 sensor).
- User-defined reporting interval.
- Suitable for operation on a three-phase, 4-wire supply at 50 or 60 Hz.
- Can be calibrated for any voltage and current (default calibration is for emonTx with 100 A CTs & UK a.c. adapter).
- **New:** Radio format support with #define selection for: JeeLib Classic, JeeLib Native & LowPowerLabs

## Limitations

Because the voltage of only one phase can be measured, the sketch must assume that the voltages of the other two phases are the same. This will, in most cases, not be true, therefore the powers calculated and recorded will be inaccurate. However, this error should normally be limited to a few percent. Voltage imbalance might be the result of unbalanced loads within your installation, or it might result from the actions of other consumers on the same supply.

If you find that one or other phase voltage is consistently wrong, you might want to consider a small adjustment to the current calibration of the affected phase. Because of the additional power required for continuous operation, a 5 V USB power supply is likely to be required, even if only the on-board RFM69CW radio is used. The 5 V power will always be required when the ESP8266 WiFi module is added.

The sketch is not compatible with the RFM12B radio module, nor the Arduino Due.

## Resources

- [Introduction to three-phase](https://learn.openenergymonitor.org/electricity-monitoring/ac-power-theory/3-phase-power)
- [Full 3-phase Firmware User Guide](emonTx_3Phase_PLL_User_Doc.pdf)

## Pre-compiled HEX files:

JeeLib Classic radio format:<br>
https://github.com/openenergymonitor/emontx3/releases/download/tx3-16-02-23/emonTx_3Phase_PLL_jeelib_classic_2_1.hex

**New:** LowPowerLabs radio format:<br>
https://github.com/openenergymonitor/emontx3/releases/download/tx3-16-02-23/emonTx_3Phase_PLL_LPL_2_1.hex

## Compile and upload using PlatformIO

PlatformIO works great from the command line. See the excellent [PlatformIO Quick Start Guide](https://docs.platformio.org/en/latest/core/installation/index.html#super-quick-mac-linux) for installation instructions.

    git clone https://github.com/openenergymonitor/emontx3
    cd emontx3/firmware/emonTx34/emonTx_3Phase_PLL
    pio run -t upload

View serial port with PlatformIO CLI

    pio device monitor
