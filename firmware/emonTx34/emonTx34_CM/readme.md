# emonTx3_CM

This firmware implements higher accuracy continuous monitoring. Continuous monitoring means that the power readings are representative of the full 10s period that they represent rather than a short snapshot (as originally implemented in the discrete sampling firmware). 

Continuous monitoring firmware was pre-loaded on emonTx3 units sold from the OpenEnergyMonitor shop as standard since 2019, unless battery operation was selected.

## Features

- Continuous Monitoring on all four EmonTxV3 CT channels and ACAC Voltage input channel.
- Real power (Watts) and cumulative energy (Watt-hours) measurement per CT channel.
- RMS Voltage measurement.
- Support for 3x DS18B20 temperature sensors.
- Support for pulse counting.
- Resulting readings transmitted via RFM radio or printed to serial.
- Serial configuration of RFM radio and calibration values.
- **New:** Radio format support with #define selection for: JeeLib Classic, JeeLib Native & **LowPowerLabs**

## Serial configuration

See [https://docs.openenergymonitor.org/emontx3/configuration.html#serial-configuration](https://docs.openenergymonitor.org/emontx3/configuration.html#serial-configuration)

## Pre-compiled HEX files:

JeeLib Classic radio format:
https://github.com/openenergymonitor/emontx3/releases/download/tx3-16-02-23/emonTx34_CM_jeelib_classic_2_4.hex

**New:** LowPowerLabs radio format:
https://github.com/openenergymonitor/emontx3/releases/download/tx3-16-02-23/emonTx34_CM_LPL_2_4.hex

## Compile and upload using PlatformIO

PlatformIO works great from the command line. See the excellent [PlatformIO Quick Start Guide](https://docs.platformio.org/en/latest/core/installation/index.html#super-quick-mac-linux) for installation instructions.

    git clone https://github.com/openenergymonitor/emontx3
    cd emontx3/firmware/emonTx34/emonTx34_CM
    pio run -t upload

View serial port with PlatformIO CLI

    pio device monitor
