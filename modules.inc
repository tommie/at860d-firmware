    ;; First because many modules depend on it.
    include "stdlib.inc"

    include "adc.inc"
    include "zerocross.inc"     ; Required by triac_pfc.
    include "triac_pfc.inc"     ; Required by airpump.
    include "airpump.inc"
    include "buttonscan.inc"
    include "buzzer.inc"
    include "led.inc"           ; Required by display.
    include "display.inc"       ; Required by timer0.
    include "triac_zcc.inc"     ; Required by heater.
    include "heater.inc"        ; Required by timer0.
    include "timer0.inc"
    include "int.inc"
    include "irq.inc"
    include "power.inc"
    include "switch.inc"        ; Required by standby.
    include "cooldown.inc"
    include "standby.inc"

    ;; Last because it may enter a fault loop on init.
    include "wdt.inc"
