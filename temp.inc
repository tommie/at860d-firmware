    ;; Temperature conversion.
    ;;
    ;; The ADC value is from a thermocouple, and should be proportional on the Kelvin scale.

    ifdef module_temp
#undefine module_temp
    endif
#define module_temp

    ifndef module_adc
    error "adc module is a dependency of temp"
    endif

    ifdef section_udata
TEMP_CONST_INIT equ 14 << 1         ; 7.1 fixed point.
TEMP_COEFF_INIT equ 75*(1 << 5)/100 ; 1.5 fixed point.

temp_value      res 2            ; 9.1 fixed point.
temp_coeff      res 1            ; 1.5 fixed point.
temp_const      res 1            ; 7.1 fixed point.
    endif ; section_udata

    ifdef section_code
temp_idle   macro
    ;; temp_value += adc_temp_value * temp_coeff + temp_const
    ;;
    ;; adc_temp_value is FP10.0.
    mov16ff         adc_temp_value, w16
    selbank         temp_coeff
    movf            temp_coeff, W
    ufpmul0816wf    w16, 5 - 1  ; To FP9.1
    add0816ff       temp_const, w16

    mov16ff         w16, temp_value
    endm

    ;; Returns the current temperature in FP9.1.
temp_get16  macro   dest
    mov16ff temp_value, dest
    endm
    endif ; section_code

    ifdef   section_init
    clr16f  temp_value
    movlw   TEMP_COEFF_INIT
    selbank temp_coeff
    movwf   temp_coeff
    movlw   TEMP_CONST_INIT
    selbank temp_const
    movwf   temp_const
    endif ; section_init

    ifdef   section_idle
    temp_idle
    endif
