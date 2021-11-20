    include "p16f887.inc"

    __config _CONFIG1, _HS_OSC & _WDTE_ON & _PWRTE_OFF & _MCLRE_ON & _CP_OFF & _CPD_OFF & _BOREN_ON & _IESO_OFF & _LVP_OFF & _DEBUG_OFF
    __config _CONFIG2, _BOR40V & _WRT_OFF

#define COOLDOWN_MAX_TEMP 20
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

    cooldown_skip_if_not_active
    goto        in_cooldown

    standby_skip_if_not_active
    goto        in_standby

    movlw       120
    airpump_setw

    movf        adc_knob_value, W
    heater_setw

    movf        adc_temp_value, W
    movwf       w16
    clrf        w16 + 1
    display_set_temp_w16

    ;movf        airpump_value, W
    movf        triaczcc_delay, W
    movwf       w16
    clrf        w16 + 1
    display_set_air_w16

    movlw       HIGH loop
    movwf       PCLATH
    goto        loop

in_cooldown:
    movlw       32
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

in_standby:
    movlw       32
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

    end
