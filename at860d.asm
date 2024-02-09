    include "p16f887.inc"

    __config _CONFIG1, _HS_OSC & _WDTE_ON & _PWRTE_OFF & _MCLRE_ON & _CP_OFF & _CPD_OFF & _BOREN_ON & _IESO_OFF & _LVP_OFF & _DEBUG_OFF
    __config _CONFIG2, _BOR40V & _WRT_OFF

BTN_UP          equ 0
BTN_DOWN        equ 1
BTN_HEAT_HNDL   equ 2
BTN_PRESET_1    equ 3
BTN_PRESET_2    equ 4
BTN_PRESET_3    equ 5
BTN_HEAT_PANEL  equ 8

;#define TEMPC_DIRECT_CONTROL
;#define DISABLE_FAULT
;#define DISABLE_COOLDOWN
;#define DISABLE_SELFTEST
;#define DISABLE_STANDBY

#define AIRPUMP_OFFSET 15                        ;; 0.8 fixed point.
#define AIRPUMP_MAX_RATIO (140 - AIRPUMP_OFFSET) ;; 0.8 fixed point.
#define BUTTONPRESS_NUM_CHANNELS 6
#define BUTTONPRESS_REPEAT_MASK ((1 << BTN_UP) | (1 << BTN_DOWN))
#define COOLDOWN_AIR 64
#define COOLDOWN_MAX_TEMP (70 << 1)     ;; 9.1 fixed point.
#define DISPLAY_CURSOR_IVAL (768 / 256) ;; In 256 ms ticks.
#define FRONTPANEL_MIN_TEMP (50 << 1)   ;; 9.1 fixed point.
#define FRONTPANEL_SET_DELAY 3000       ;; In 1 ms ticks.
#define KNOB_HYSTERESIS 4               ;; In ADC units.
#define SELFTEST_TIMEOUT (10000 / 128)  ;; In 128 ms ticks.
#define SELFTEST_ACCEPT_STATE SELFTEST_FAST_TESTS
#define STANDBY_AIR COOLDOWN_AIR
#define STANDBY_DELAY (20000 / 128)     ;; In 128 ms ticks.
#define STANDBY_MAX_TEMP COOLDOWN_MAX_TEMP
#define TRIACPFC_NUM_CHANNELS 1
#define TRIACZCC_NUM_CHANNELS 1
#define TRIACZCC_NUM_FRAC_BITS 5
#define TRIACZCC_NUM_EXTRA_BITS 3
    ifndef TEMPC_DIRECT_CONTROL
#define TEMPCONTROL_MIN_TEMP (35 << 1)  ;; 9.1 fixed point.
#define TEMPCONTROL_MAX_TEMP (500 << 1) ;; 9.1 fixed point.
    else
#define TEMPCONTROL_MIN_TEMP 0
#define TEMPCONTROL_MAX_TEMP ((1 << (TRIACZCC_NUM_FRAC_BITS + TRIACZCC_NUM_EXTRA_BITS)) - 1) << 1
    endif   ; TEMPC_DIRECT_CONTROL
#define FAULT_MIN_TEMP (TEMPCONTROL_MIN_TEMP / 2) ;; 9.1 fixed point.
#define FAULT_MAX_TEMP (550 << 1)                 ;; 9.1 fixed point.
#define FAULT_HEATER_DELAY (10000 / 128)          ;; In 128 ms units.

    udata

#define section_udata
    include "modules.inc"
#undefine section_udata

    udata_shr

#define section_udata_shr
    include "modules.inc"
#undefine section_udata_shr

    org     0x2100

#define section_eedata
    include "modules.inc"
#undefine section_eedata

    code

#define section_code
    include "modules.inc"
#undefine section_code

    org     0x00
reset:
    nop
    movlw   HIGH start
    movwf   PCLATH
    goto    start

    org     0x04
irq:
    irq_enter

#define section_irq
    include "modules.inc"
#undefine section_irq

    irq_retfie

start:
#define section_init
    include "modules.inc"
#undefine section_init

    ifdef               DISABLE_SELFTEST
    tempc_set_active    1
    endif

    irq_enable

loop:
#define section_idle
    include "modules.inc"
#undefine section_idle

    movlw       HIGH loop
    movwf       PCLATH
    goto        loop

    end
