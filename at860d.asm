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

#define BUTTONPRESS_NUM_CHANNELS 6
#define BUTTONPRESS_REPEAT_MASK ((1 << BTN_UP) | (1 << BTN_DOWN))
#define COOLDOWN_MAX_TEMP 20    ;; In 128 ms ticks.
#define DISPLAY_CURSOR_IVAL 3   ;; In 256 ms ticks.
#define SELFTEST_TIMEOUT (10000 / 128)
#define SELFTEST_ACCEPT_STATE SELFTEST_FAST_TESTS
#define TRIACPFC_NUM_CHANNELS 1
#define TRIACZCC_NUM_CHANNELS 1
#define TRIACZCC_NUM_FRAC_BITS 5
#define TRIACZCC_NUM_EXTRA_BITS 3

;; In 20 s in 128 ms ticks.
#define STANDBY_DELAY (20000 / 128)

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

    irq_enable

loop:
#define section_idle
    include "modules.inc"
#undefine section_idle

    ifndef      DISABLE_COOLDOWN
    movlw       HIGH in_cooldown
    movwf       PCLATH
    cooldown_skip_if_not_active
    goto        in_cooldown
    endif

    ifndef      DISABLE_SELFTEST
    movlw       HIGH in_selftest
    movwf       PCLATH
    selftest_skip_if_passed
    goto        in_selftest
    endif

    ifndef      DISABLE_STANDBY
    movlw       HIGH in_standby
    movwf       PCLATH
    standby_skip_if_not_active
    goto        in_standby
    endif

    fp_display

    movlw       HIGH loop
    movwf       PCLATH
    goto        loop

    ifndef      DISABLE_SELFTEST
in_selftest:
    movlw       LSA & LSE & LSF             ; "T"
    movwf       display_buf + 2
    movwf       display_buf + 0
    movlw       LSA & LSC & LSD & LSF & LSG ; "S"
    movwf       display_buf + 1

    movf        selftest_state, W
    movwf       w16
    movf        selftest_state + 1, W
    andlw       0x03
    movwf       w16 + 1
    display_set_air_w16

    selftest_goto_if_failed failed_selftest

    movlw       ~0
    movwf       display_buf + 6

    movlw       HIGH loop
    movwf       PCLATH
    goto        loop

failed_selftest:
    movlw       LS2A & LS2E & LS2F & LS2G ; "F"
    movwf       display_buf + 6

    ;; if (!swpower_get()) selftest_reset()
    movlw       HIGH failed_selftest_reset
    movwf       PCLATH
    swpower_get STATUS, C
    btfss       STATUS, C
    goto        failed_selftest_reset

    movlw       HIGH loop
    movwf       PCLATH
    goto        loop

failed_selftest_reset:
    selftest_reset

    movlw       HIGH loop
    movwf       PCLATH
    goto        loop
    endif

    ifndef      DISABLE_COOLDOWN
in_cooldown:
    movlw       64
    airpump_setw

    clrw
    heater_setw

    movlw       LSA & LSD & LSE & LSF               ; "C"
    movwf       display_buf + 2
    movlw       LSA & LSB & LSC & LSD & LSE & LSF   ; "O"
    movwf       display_buf + 1
    movwf       display_buf + 0
    movlw       LS2D & LS2E & LS2F                  ; "L"
    movwf       display_buf + 6

    movlw       ~0
    movwf       display_buf + 3
    movwf       display_buf + 4
    movwf       display_buf + 5

    movlw       HIGH loop
    movwf       PCLATH
    goto        loop
    endif

    ifndef      DISABLE_COOLDOWN
in_standby:
    movf        adc_temp_value, W
    sublw       COOLDOWN_MAX_TEMP
    movlw       64
    btfsc       STATUS, C
    clrw
    airpump_setw

    clrw
    heater_setw

    movlw       LSA & LSC & LSD & LSF & LSG ; "S"
    movwf       display_buf + 2
    movlw       LSD & LSE & LSF             ; "L"
    movwf       display_buf + 1
    movlw       LSA & LSB & LSE & LSF & LSG ; "P"
    movwf       display_buf + 0

    movlw       ~0
    movwf       display_buf + 3
    movwf       display_buf + 4
    movwf       display_buf + 5
    movwf       display_buf + 6

    movlw       HIGH loop
    movwf       PCLATH
    goto        loop
    endif

    end
