# An Unofficial Firmware for the Atten AT860D

[![Build](https://github.com/tommie/at860d-firmware/actions/workflows/build.yaml/badge.svg)](https://github.com/tommie/at860d-firmware/actions/workflows/build.yaml)

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

## Features

The firmware is not doing exactly what the original does, esp. not the calibration mode.
Most of the day-to-day is the same.

* Three presets.
* Closed-loop temperature control.
* Keyed set-up mode that allows changing temperature and presets.
* Keyed calibration mode for adjusting temperature readings.
* Standby mode.
* Cooldown mode.
* Self-test on start-up.

### Compared to the Original

* Smoother temperature control (my desk light isn't flickering anymore!)
* More obvious differences between set-up and calibration modes.
* Cooling down to 70 °C instead of 150 °C, for household safety.
* The knob does not cause values to jump as soon as you touch it.
* The lowest possible temperature is 50 °C (instead of 150 °C.)
  * Though note that the temperature reading is not very accurate below 150 °C.
* The airpump scale goes up to 125.
* Start-up self-test.
* Since it's open-source, you can change constants.
* The buzzer is never used.

### Missing Features

* The calibration mode changes coefficient and offset separately, which isn't very user-friendly.
* Fahrenheit display.

### Status

The firmware is expected to be working well, but is only running on my device.
I'm curious to hear if it's working, or not, for you.

## Building and Uploading

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

Note that while the display may turn on via the programming header (and drawing 70 mA,) the amplifier for the temperature sensor will not work, and the temperature is reported as very high (>600 °C.)
It requires a dual-supply 5 V.
If you want to experiment without mains voltage on the PCB, the easiest way is to feed the 7.5 V transformer directly (normally attached to CN3.)
This will also make the zero-cross detector work.

### Issues with gputils 1.5.0

It won't build with 1.5.0, which is sad because it has banksel/pagesel optimizations that would be useful.
Those optimizations can save 30% of space.
There is something wrong about the use of org/code that causes gpasm to complain about overwriting previous address contents: 0x0004.
This is likely about our mixed used of code and org.
Splitting `section_code` into macros and moving all functions to after the idle loop isn't enough:
it just moves the error to 0x000E.
At other times, it complains about non-contiguous `code` sections.
In the end, I had no incentive to spend more time on it.
1.4.0 is what is in Ubuntu.

## Usage

## Normal Mode

When the cal/set card is not inserted.

**Note** that normal mode is meant for professional production environments.
If you don't have a supervisor, you probably always want to run in *set-up mode*.

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

## Set-Up Mode

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

## Code Structure

The `at860d.asm` file separates the program into sections:

* udata
* udata_shr
* eedata
* code
* irq
* init
* idle

The `init` and `idle` sections are what Arduino call `setup` and `loop`.
For each section, the `modules.inc` file is included with the appropriate `section_*` flag defined.
The modules are listed in dependency order, and contain `ifdef/endif` blocks for the sections they care about.
See [tommie/dios](https://github.com/tommie/dios) for an extension to this idea of modular PIC assembly.

The best place to start discovering is probably `modules.inc`, since it shows if a module is part of the HAL, an algorithm or high-level function.
Most code is implemented inside macros to allow using local labels.

## Hardware

### Connectors

* CN1 - Mains In
* CN2 - Mains Switch
* CN3 - Mains 7.5V-Transformer primary
* CN4 - Mains Heater
* CN5 - Mains Airpump
* CN6 - 7.5V Transformer secondary
* CN8 - Handle
* CN9 - ICSP/Programming

### CN8 - Handle

Pin 1 is at the bottom:

```
8. Temp (Gnd)
7. Temp
6. Standby (Gnd)
5. Standby
4. Up
3. Down
2. Heat
1. Button common
```

## MCU

1.  AN0 - Temp
2.  RA1 - N/C
3.  RA2 - Photodiode Set
4.  AN3 - Knob
5.  RA4 - Switch Standby in handle
6.  RA5 - Power switch
7.  RB0 - Zero cross detect
8.  RB1 - Photodiode Calibration
9.  RB2 - Button scan in (handle, up; K1, up; K4, preset 1)
10. RB3 - Button scan in (handle, down; K2, down; K5, preset 2)
11. RB4 - Button scan in (handle, heat; K3, heat; K6, preset 3)
12. RB5 - Button scan out (K3, heat)
13. RB6 - Button scan out (K4, preset 1; K5, preset 2; K6, preset 3) (ICSPCLK)
14. RB7 - Button scan out (handle; K1, up; K2, down) (ICSPDAT)
15. RC0 - Buzzer
16. RC1-7 - LED digit, CA
17. RD0-7 - LED segment
18. RE0 - Power optocoupler (MOC3083, zero-cross detection)
19. RE1 - Air optocoupler (MOC3023, no zero-cross detection)
20. RE2 - Heater optocoupler and LED (MOC3083, zero-cross detection)
21. RE3 - MCLR
