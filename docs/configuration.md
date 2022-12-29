# Configuration

## DIP Switch

The on board DIP switch can be used as a convenient way to change both the node ID and the voltage calibration of the emonTx3.


Multiple emonTx unit's can operate on a single network posting to a single emonBase web-connected base station, each emonTx on the same network group must have an unique node ID. If more than two emonTx's are required on the same network then further nodeID values can be set via RF node ID serial config.

The following image shows the DIP switch configuration looking at the emonTx with the CT sensor inputs at the top of the board. 

```{image} img/emontx_dipswitch.jpg
:width: 400px
:align: right
```

<br>

Move the top switch D9 to the left to select USA ACAC Voltage calibration. 

Move the bottom switch D8 to the left to select RF node ID 16 rather than 15.

<br>
<br>


## Serial Configuration

It's possible to set the emonTx radio settings, sensor calibration and other properties over serial. See [Github PDF: Configuration of RF Module & on-line calibration](https://github.com/openenergymonitor/EmonTxV3CM/blob/master/Config.pdf) for full details. If a custom node ID is set, a corresponding node decoder needs to be in place in emonhub.conf to decode the EmonTx radio packet data. See [emonhub.conf configuration guide](https://github.com/openenergymonitor/emonhub/blob/emon-pi/configuration.md).

Note: When using the Arduino IDE to set properties of the firmware via serial, ensure the serial monitor is set to `NL/CR` and the baud rate is set to `115200`. The serial monitor may need to be restarted after changing the line ending selection.
