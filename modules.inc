    ;; First because many modules depend on it.
    include "stdlib.inc"

    include "adc.inc"
    include "airpump.inc"
    include "buttonscan.inc"
    include "buzzer.inc"
    include "zerocross.inc"     ; Required by triac.
    include "triac.inc"         ; Required by int.
    include "led.inc"           ; Required by display.
    include "display.inc"       ; Required by timer0.
    include "heater.inc"        ; Required by timer0.
    include "timer0.inc"        ; Required by zerocross.
    include "int.inc"
    include "irq.inc"
    include "power.inc"
    include "switch.inc"

    ;; Last because it may enter a fault loop on init.
    include "wdt.inc"
