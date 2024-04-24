# NickelClock - Show time while reading

A Nickel mod to show time while reading on Kobo ereaders.

This mod adds a clock to the header or footer while reading a book. It is an 
alternative to [miniclock](https://www.mobileread.com/forums/showpost.php?p=3762123&postcount=6) 
and [NanoClock](https://github.com/NiLuJe/NanoClock).

### Examples

*Header - Left*
![header left](./images/header-left.png)

*Header - Right*
![header right](./images/header-right.png)

*Footer - Left*
![footer left](./images/footer-left.png)

*Footer - Right*
![footer right](./images/footer-right.png)

## Install or Upgrade NickelClock

1. Download the `NickelClock-<version>.zip` file from the [latest](https://github.com/shermp/NickelClock/releases/latest) 
   release. Extract the `KoboRoot.tgz` file from the downloaded file 
   (note: Mac OS X may extract this by default on download).
2. Copy `KoboRoot.tgz` to the `.kobo` directory on your Kobo, and disconnect 
   it from your computer. The Kobo will reboot automatically.

## Configure NickelCLock

*NickelClock 0.2.0 used a different settings format. When upgrading, settings are 
automatically migrated.*

The clock and battery may be positioned independently in one of four locations: 

**Left header**, **Right header**, **Left footer**, **Right footer**

Positioning and other settings are saved in `.adds/nickelclock/settings.ini`, 
and the default settings file is as follows:

```ini
[General]
Margin=Auto

[Battery]
BatteryType=Level
Enabled=false
Placement=Header
Position=Right
LevelTemplate=%1%

[Clock]
Enabled=true
Placement=Header
Position=Right

```
The following settings may be set. **Note that entries are case sensitive**:

### [General] settings

|Setting|Values|
|-------|------|
|`Margin`|`Auto`, or any whole number greater than zero, up to a quarter of your screen width.|

### [Clock] and [Battery] settings

|Setting|Values|
|-------|------|
|`Placement`|`Header`, `Footer`|
|`Position` |`Left`, `Right`|
|`Enabled`  |`true`, `false`|

### [Battery] settings

|Setting|Values|
|-------|------|
|`BatteryType`|`Level`, `Icon`, `Both`|
|`LevelTemplate`|Any string that contains `%1`|

The battery icon is not compatible with dark mode, the icon is not inverted.

Setting both clock and battery level to the same placement and position is 
not supported, and the result will be neither showing.

No other customisation is available at this time.

If you have disabled the header and/or footer in the reading settings, the 
clock or battery may not show.

## Compatibility

NickelClock should be compatible with any Kobo device running a recent 4.x 
firmware release. It has currently been tested to work on firmware 4.33.

## Uninstall NickelClock

To uninstall NickelClock, simply delete the `uninstall` file from the
`.adds/nickelclock` directory, then restart your Kobo.

## FAQ

### How does this differ from MiniClock/NanoClock?

NickelClock works in a fundamentally different way to MiniClock or NanoClock. 
They directly print to the screen wheras NickelClock creates a Widget that 
Kobo's software displays.

MiniClock and NanoClock are much more configurable than NickelClock is. If 
you want precise control over the positioning and appearance of your clock, 
NickelClock is probably not what you want to use.

MiniClock/NanoClock have known stability issues with newer devices, especially 
the Kobo Libra2. NickelClock should not have such stability issues.
