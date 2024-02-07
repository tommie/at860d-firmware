    ;; First because many modules depend on it.
    include "stdlib.inc"
    include "fixedp.inc"

    include "adc.inc"
    include "buttonscan.inc"
    include "buzzer.inc"
    include "led.inc"           ; Required by display.
    include "display.inc"       ; Required by timer0.
    include "timer0.inc"
    include "zerocross.inc"     ; Required by triac_pfc.
    include "triac_pfc.inc"     ; Required by airpump.
    include "airpump.inc"
    include "triac_zcc.inc"     ; Required by heater.
    include "heater.inc"        ; Required by timer1.
    include "timer1.inc"        ; Required by int.
    include "int.inc"
    include "irq.inc"
    include "power.inc"
    include "switch.inc"        ; Required by standby.
    include "buttonpress.inc"   ; Required by frontpanel.
    include "knob.inc"          ; Required by frontpanel.
    include "eeprom.inc"        ; Required by frontpanel.
    include "temp.inc"          ; Required by tempcontrol.
    include "pid.inc"           ; Required by tempcontrol.
    include "tempcontrol.inc"   ; Required by userconfig.
    include "userconfig.inc"    ; Required by standby.
    ifndef  DISABLE_COOLDOWN
    include "cooldown.inc"
    endif
    ifndef  DISABLE_SELFTEST
    include "selftest.inc"
    endif
    ifndef  DISABLE_STANDBY
    include "standby.inc"
    endif
    include "frontpanel.inc"

    ;; Last because it may enter a fault loop on init.
    include "wdt.inc"
