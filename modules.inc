    ;; Algorithms
    include "stdlib.inc"
    include "fixedp.inc"
    include "pid.inc"

    ;; Hardware drivers
    include "adc.inc"
    include "buttonscan.inc"
    include "buzzer.inc"
    include "eeprom.inc"
    include "irq.inc"
    include "led.inc"
    include "power.inc"
    include "switch.inc"

    ;; Tasks using the timer0 ISR
    include "display.inc"
    include "timer0.inc"

    ;; Tasks using the timer1/int ISRs
    ;; (Timer1 is synchronized to zero-crossings.)
    include "zerocross.inc"
    include "triac_pfc.inc"
    include "triac_zcc.inc"
    include "airpump.inc"
    include "heater.inc"
    include "timer1.inc"
    include "int.inc"

    ;; Best-effort tasks
    include "buttonpress.inc"
    include "knob.inc"
    include "temp.inc"
    include "fault.inc"
    include "tempcontrol.inc"
    include "userconfig.inc"

    ;; High-level modes of operation
    ifndef  DISABLE_COOLDOWN
    include "cooldown.inc"
    endif
    ifndef  DISABLE_SELFTEST
    include "selftest.inc"
    endif
    ifndef  DISABLE_STANDBY
    include "standby.inc"
    endif
    ifndef  DISABLE_CALIBRATION
    include "calibration.inc"
    endif
    include "frontpanel.inc"
    include "wdt.inc" ; Last because it may enter a fault loop on init.
