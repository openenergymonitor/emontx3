# Adding to an existing install

Additional [EmonTx energy monitoring nodes](/technical/emontx) can be added to both an emonPi or a emonBase system to:

- Expand the number of AC circuits that can be measured.<br>(Each EmonTx adds +4x CT sensor inputs)
- Monitor circuits at different locations in a building
- Monitor circuits where an AC socket is not available (using battery power)
- Monitor 3-phase power (with 3-phase emonTx firmware, see [EmonTx Technical](/technical/emontx))

Data is transmitted to the emonPi or emonBase via low power 433MHz radio as standard, or alternatively using the [ESP8266 WiFi adapter](esp8266.md) via WiFi for applications where the 433 MHz radio range is insufficient. 

![emontx](img/emontx.jpg)

## 1. Adding one emonTx to an emonPi system

If you have already setup an emonPi following the [emonPi installation guide](emonpi/install.md). The steps for installing an emonTx are covered in the [emonTx and emonBase installation guide](emontx3/install.md). The emonPi in this instance works in much the same way as the emonBase.

## 2. Adding more than one emonTx to an emonPi or emonBase system

In addition to standard emonTx installation as covered in the [emonTx and emonBase installation guide](/setup/install-emontx): when using 2 or more emonTx units with the same base-station **the node ID on each unit needs to be unique**. 

The nodeID can be selected at time of purchase or set using the on-board DIP switch to toggle. If more than two emonTx's are required on the same network then further nodeID values can be set via RF node ID serial config.

```{image} img/emontx_dipswitch.jpg
:width: 400px
:align: right
```

The image on the right shows the DIP switch configuration looking at the emonTx with the CT sensor inputs at the top of the board. Move the top switch D9 to the left to select USA ACAC Voltage calibration. **Move the bottom switch D8 to the left to select RF node ID 16 rather than 15.**

**Serial Configuration**<br>

It's possible to set the emonTx radio settings, sensor calibration and other properties over serial. See [Github PDF: Configuration of RF Module & on-line calibration](https://github.com/openenergymonitor/EmonTxV3CM/blob/master/Config.pdf) for full details. If a custom node ID is set, a corresponding node decoder needs to be in place in emonhub.conf to decode the emonTx radio packet data. See [emonhub.conf configuration guide](https://github.com/openenergymonitor/emonhub/blob/emon-pi/configuration.md).

Note: When using the Arduino IDE to set properties of the firmware via serial, ensure the serial monitor is set to `NL/CR` and the baud rate is set to `115200`. The serial monitor may need to be restarted after changing the line ending selection.
