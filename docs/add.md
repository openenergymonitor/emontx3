# Adding to an existing install

Additional [EmonTx energy monitoring nodes](../emontx3/add.md) can be added to both an emonPi or a emonBase system to:

- Expand the number of AC circuits that can be measured.<br>(Each EmonTx adds +4x CT sensor inputs)
- Monitor circuits at different locations in a building
- Monitor circuits where an AC socket is not available (using battery power)
- Monitor 3-phase power (with 3-phase emonTx firmware, see [EmonTx Technical](../emontx3/add.md))

Data is transmitted to the emonPi or emonBase via low power 433MHz radio as standard, or alternatively using the [ESP8266 WiFi adapter](esp8266.md) via WiFi for applications where the 433 MHz radio range is insufficient. 

![emontx](img/emontx.jpg)

## 1. Adding one emonTx to an emonPi system

If you have already setup an emonPi following the [emonPi installation guide](../emonpi/install.md). The steps for installing an emonTx are covered in the [emonTx and emonBase installation guide](../emontx3/install.md). The emonPi in this instance works in much the same way as the emonBase.

## 2. Adding more than one emonTx to an emonPi or emonBase system

In addition to standard emonTx installation as covered in the [emonTx and emonBase installation guide](../emontx3/install.md): when using 2 or more emonTx units with the same base-station **the node ID on each unit needs to be unique**. See [Configuration](configuration.md) for details on how to change the nodeid.

