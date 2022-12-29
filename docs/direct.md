# emonTx3 direct serial

EmonTx3 data can also be read directly over serial or via a USB to UART cable. When connected to an emonPi/emonBase or RaspberryPi, the data can be read and forwarded directly to emoncms using the emonHub software utility.

## Using a USB to UART cable

![emontx3_uart.png](img/emontx3_uart.png)

**EmonHub interfacer configuration**<br>
To use emonHub to read the emonTx3 data, add the following emonHub OEM interfacer to the EmonHub configuration file in the interfacers section:

    [[USB0]]
        Type = EmonHubOEMInterfacer
        [[[init_settings]]]
            com_port = /dev/ttyUSB0
            com_baud = 115200
        [[[runtimesettings]]]
            pubchannels = ToEmonCMS,
            nodename = emonTx3

## Direct Raspberry Pi GPIO

Both the Raspberry Pi and emonTx v3 run at 3.3V so the serial Receive and Transmit lines can be connected directly. The 5V power rail from the Raspberry Pi can be supplied to the emonTx which is then stepped down to to 3.3V by the emonTx voltage regulator. 5V is provided by the red wire (see photo). The ground connection is the black wire and the serial data going from the emonTx to the Raspberry Pi is the green wire. Wire for serial data going the opposite direction (Pi to emonTx) has not been connected in this example but could be added if two-way communication is required.

```{note}
On the PCB and schematic, the Tx and Rx pins are labelled according to the connections on the Programmer, meaning that data is received by the emonTx on the Tx pin, and transmitted by the emonTx from the Rx pin.
```

![](img/emontx3_direct_serial.png)

**EmonHub interfacer configuration**<br>
To use emonHub to read the emonTx3 data, add the following emonHub OEM interfacer to the EmonHub configuration file in the interfacers section:

    [[AMA0]]
        Type = EmonHubOEMInterfacer
        [[[init_settings]]]
            com_port = /dev/ttyAMA0
            com_baud = 115200
        [[[runtimesettings]]]
            pubchannels = ToEmonCMS,
            nodename = emonTx3

