# An Unofficial Firmware for the Atten AT860D

AT860D is a hot air soldering station.
Mine ended up killing its own microcontroller somehow:
the flash ROM program memory turned itself into RAM, and was wiped.
After trying to re-program it, I realized the chip itself was toast.

So, there I was without a hot air soldering station and firmware to go with it.
I bought a new PIC16F887, started reverse-engineering the hardware and wrote a new firmware for it.

I wanted to use SDCC, but its PIC16F-support is not great.
And I wanted to keep the firmware open, so the proprietary C compilers out there were not in scope.
Assembler it is...
And here we are, a few years later.

## Building

Requires gputils 1.4.0.

```console
$ git clone ...
$ cd ...
$ make
```

The output file to upload is `at860d.hex`.

The programming header is on the left side (looking at the front of the device,) just below the connector for the handle.
It's called *CN9* and has the following pinout (from the top):

```
6. Gnd
5. VDD
4. PGC
3. PGD
2. N/C
1. MCLR
```

You need a PIC programmer capable of "high voltage" programming to program the device, because the PGC/PGD pins are used as normal I/O.

Note that while the display may turn on via the programming header (and drawing 70 mA,) the amplifier for the temperature sensor will not work, and the temperature is reported as very high (>600 *C.)
It requires a dual-supply 5 V.
If you want to experiment without mains voltage on the PCB, the easiest way is to feed the 7.5 V transformer directly (normally attached to CN3.)
This will also make the zero-cross detector work.

## Features

The firmware is not doing exactly what the original does, esp. not the calibration mode.
Most of the day-to-day is the same.

* Three presets.
* Closed-loop temperature control.
* Keyed setup mode that allows changing temperature and presets.
* Keyed calibration mode for adjusting temperature readings.
* Standby mode.
* Cooldown mode.
* Self-test on start-up.

### Missing Features

* Fahrenheit display

## Usage

## Normal Mode

When the cal/set card is not inserted.

* The current temperature and the set airflow is normally displayed.
* The up/down buttons and knob set the airflow.
  * A blinking dot indicates the currently selected value.
  * Holding the up/down buttons allows moving fast.
* Pressing one of the three preset buttons loads that preset.

## The Knob

Turning the knob does nothing until the current value is passed.
This is to avoid values jumping when you start turning.
Thus, you may have to rock it back and forth to make it pick up.

## Standby

A few seconds after the handle is placed in the cradle, the heater is disabled and airflow is reduced.
When the temperature drops below a threshold, the airflow is disabled and "SLP" shows on the air display.
Lifting the handle enables the heater again.

## Cooldown

When turning the device off while at a high temperature, the airpump continues to run, showing "COL" on the air display.
When the temperature drops below a threshold, the device turns itself off.

## Faults

Check your hardware if any of these occur:

* If the temperature sensor indicates temperatures are too high, a sensor error is reported as "S-E" on the temp display.
* If the heater should be turned on, but the measured temperature is low, a heater error is reported as "H-E" on the temp display.

This indicates a program (or programming) bug:

* If the microprocessor fails to do its thing, a watchdog timer will expire, displaying a "t".

## Set Mode

When the cal/set card is inserted with "SET" up.

This mode works like the normal mode, with these differences:

* By default, the up/down buttons and knob set the temperature.
* Pressing the heat button switches to setting the airflow (range 0-125.)
  * A blinking dot indicates the currently selected value.
* Holding one of the three preset buttons overwrites that preset.
  * The temp unit display shows a "P" for a few seconds as confirmation.

## Calibration Mode

When the cal/set card is inserted with "CAL" up.

* The air display shows "CAL".
* The temp display shows the set temperature.
* By default, the up/down buttons set the temperature of the device.
* Pressing Preset 1 enters coefficient mode.
  * The air display shows "AT1" and the temp display shows the current temperature.
  * The up/down buttons changes the temperature value coefficient.
* Pressing Preset 2 enters offset constant mode.
  * The air display shows "AT2" and the temp display shows the current temperature.
  * The up/down buttons changes the temperature value offset.
* Pressing the heat button returns to the "CAL" mode.
* Holding the heat button saves the new calibration values into persistent memory.
  * The air display shows "SET" for a few seconds as confirmation.

## Self-Test

The device checks its inputs on start.
Normally these tests pass in less than a second.
The temp display shows "TST" while running the tests.
If a failure occurs, "TSTF" is displayed.

The air display shows the tests that have passed as a decimal number.
See the bit definitions in selftest.inc.
