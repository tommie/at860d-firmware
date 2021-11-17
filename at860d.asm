    include "p16f887.inc"

    __config _CONFIG1, _HS_OSC & _WDTE_ON & _PWRTE_OFF & _MCLRE_ON & _CP_OFF & _CPD_OFF & _BOREN_ON & _IESO_OFF & _LVP_OFF & _DEBUG_OFF
    __config _CONFIG2, _BOR40V & _WRT_OFF

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

    movf        adc_knob_value, W
    movwf       w16
    clrf        w16 + 1
    display_set_temp_w16

    triac_set   1, adc_knob_value

    movf        triac_delay + 1, W
    movwf       w16
    clrf        w16 + 1
    display_set_air_w16

    movlw       HIGH loop
    movwf       PCLATH
    goto        loop

    end
