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
TEMP_CONST_INIT equ (35 << 1) + 0   ; 7.1 fixed point.
TEMP_COEFF_INIT equ (0 << 4) + 12   ; 2.4 fixed point.

temp_value  res 2               ; 10.6 fixed point.
temp_coeff  res 1               ; 2.4 fixed point.
temp_const  res 1               ; 7.1 fixed point.
    endif

    ifdef section_code
temp_idle   macro
    ;; temp_value += (adc_temp_value * temp_coeff + temp_const - temp_value) / 2^4
    ;;
    ;; adc_temp_value is 10.0 fixed point.
    mov16ff adc_temp_value, w16
    selbank temp_coeff
    movf    temp_coeff, W
    mul16wf w16
    rr16fl  w16, 4 - 1, 0       ; To FP15.1
    add16cf w16

    add0816ff   temp_const, w16
    rl16fl      w16, 6 - 1, 0       ; To FP10.6

    sub16ff     temp_value, w16
    divp2s16lf  w16, 4
    add16cf     w16

    add16ff w16, temp_value
    endm

    ;; Returns the current temperature in FP9.1.
temp_get16  macro   dest
    mov16ff temp_value, dest
    rr16fl  dest, 6 - 1, 0
    add16cf dest
    endm
    endif

    ifdef section_init
    clr16f  temp_value
    movlw   TEMP_COEFF_INIT
    selbnkc temp_coeff
    movwf   temp_coeff
    movlw   TEMP_CONST_INIT
    selbnkc temp_const
    movwf   temp_const
    endif

    ifdef section_idle
    temp_idle
    endif
