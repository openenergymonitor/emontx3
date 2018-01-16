# Default emonTx Firmware

## Firmware

**All pre-assembled emonTx V3's are shipped with this firmware**

**Key features:**

* Detection of AC-AC adapter sets Apparent Power / Real Power Sampling accordingly
* Detection of battery / USB 5V or AC > DC power method and sets sleep mode accordingly
* Detection of CT connections and samples only from the channels needed
* Detection of remote DS18B20 temperature sensor connection
* Low power battery operation supported
* DIP switch 1 (closes to RF module) to select node ID. (Switch off node ID =10, switch on node ID = 9)
* DIP switch 2 to select UK/EU or USA AC-AC adapter calibration (Switch off = UK/EU, Switch on = USA)
* [Serial RF nodeID config](https://community.openenergymonitor.org/t/emontx-v3-configure-rf-settings-via-serial-released-fw-v2-6-0/2064)
*

[**Change Log**](https://github.com/openenergymonitor/emontx3/blob/master/firmware/changelog.md)

***

# Compile & Upload

- A [5v USB to UART cable](https://shop.openenergymonitor.com/programmers) is required to upload firmware to emonTx

## Option 1.)

Upload Pre-compiled firmware using [emonUpload tool](https://github.com/openenergymonitor/emonupload)

***

## Option 2.)

Compile and upload firmware using [PlatformIO](https://platformio.org)

### Install patformio (if needed)

See [platformio install quick start](http://docs.platformio.org/en/latest/installation.html#super-quick-mac-linux)

Recommended to use install script which may require sudo:

`python -c "$(curl -fsSL https://raw.githubusercontent.com/platformio/platformio/master/scripts/get-platformio.py)"`

### Compile

    $ pio run

### Upload

    $ pio run -t upload

### Test (optional)

See [PlatfomIO unit test docs](http://docs.platformio.org/en/feature-platformio-30/platforms/unit_testing.html#example). Requires PlatformIO 3.x

    $ pio test


***

## Option 3.)

Compile with Arduino IDE
