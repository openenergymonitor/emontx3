# Firmware

The emonTx3 firmware is based on Arduino. Alternative or customised firmware sketches can be uploaded using Arduino IDE or PlatformIO and a USB to UART cable.

## Available Firmware

### [emonTxV3CM Continuous Sampling firmware](https://github.com/openenergymonitor/EmonTxV3CM)

**Released 2019:** This firmware provides higher accuracy continuous monitoring. Continuous monitoring means that the power readings are representative of the full 10s period that they represent rather than a short snapshot. Pre-loaded as standard since 2019, unless battery operation selected.

### [emonTx Discreet Sampling firmware](https://github.com/openenergymonitor/emontx3/tree/master/firmware)

The original emonTx firmware, this performs power measurement in short discreet snapshots ~300ms long per CT channel at 50Hz per 10s period. This makes it possible for the emonTx to go to sleep inbetween readings enabling battery powered operation but is less accurate.<br>

**Indicator LED:** Illuminates solid for a 10 seconds on first power up, then flashes multiple times to indicate an AC-AC waveform has been detected (if powering via AC-AC adapter). Flashes once every 10s to indicate sampling and RF transmission interval.

### [3-phase firmware](https://github.com/openenergymonitor/emontx-3phase)

This firmware is intended for use on a 3-phase, 4-wire system and implements continuous monitoring as above. Because the voltage of only one phase can be measured, the firmware must assume that the voltages of the other two phases are the same. This will, in most cases, not be true, therefore the powers calculated and recorded will be inaccurate. However, this error should normally be limited to a few percent.

- [Learn: Introduction to three-phase](https://learn.openenergymonitor.org/electricity-monitoring/ac-power-theory/3-phase-power)
- [3-phase Firmware](https://github.com/openenergymonitor/emontx-3phase) 
- [Full 3-phase Firmware User Guide](https://github.com/openenergymonitor/emontx-3phase/blob/master/emontx-3-phase-userguide.pdf)

## Updating firmware using an emonPi/emonBase

The easiest way of updating the emonTx3 firmware is to connect it to an emonPi or emonBase with a USB to UART cable and then use the firmware upload tool available at `Setup > Admin > Update > Firmware`.

The example images below show the earlier [Wicked Device / OpenEnergyMonitor Programmer](../electricity-monitoring/programmers/wicked-device.md). The programmer is the small board that plugs in to the emonTx3 on the 6-pin UART header. The [newer programmer](../electricity-monitoring/programmers/ftdi-programmer.md) currently available in the shop needs to be orientated the other way around. **Make sure that GND on the programmer matches up with GND on the emonTx3 board.**

Refresh the update page after connecting the USB cable. You should now see port ttyUSB0 appear in the ‘Select port` list.

![emontx3_uart.png](img/emontx3_uart.png)

Select port: `ttyUSB0`, Hardware: `emonTx`, Radio format: `RFM69 JeeLib Classic`, Firmware: As required, see above.

![emonTx3_firmware_upload.png](img/emonTx3_firmware_upload.png)

Click `Update Firmware` to upload the firmware.

## How to compile and upload firmware

### Arduino IDE

- To compile and upload the emonTxV3CM Continuous Sampling firmware, please see Arduino IDE section here: [https://github.com/openenergymonitor/EmonTxV3CM](https://github.com/openenergymonitor/EmonTxV3CM).

### PlatformIO Command Line

PlatformIO works great from the command line. See the excellent [PlatformIO Quick Start Guide](https://docs.platformio.org/en/latest/core/installation/index.html#super-quick-mac-linux) for installation instructions.

**Compile and upload emonTxV3CM Continuous Sampling firmware**

    git clone https://github.com/openenergymonitor/EmonTxV3CM
    cd EmonTxV3CM
    pio run -t upload

If the emonTx3 is connected via RPi serial, the firmware should be complied with:
   
    pio run -v -e emontx_pi
    
**Compile and upload emonTx3 Discrete Sampling firmware**

    git clone https://github.com/openenergymonitor/EmonTx3
    cd EmonTx3/firmware
    pio run -t upload
    
**Compile and upload emonTx 3-phase firmware**

    git clone https://github.com/openenergymonitor/emontx-3phase
    cd emontx-3phase
    pio run -t upload

**View serial port with PlatformIO CLI**

    pio device monitor
