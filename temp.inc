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
temp_value  res 2               ; 8.0 fixed point.
temp_coeff  res 1               ; 4.4 fixed point.
temp_const  res 1               ; 8.0 fixed point.
    endif

    ifdef section_code
temp_idle   macro
    banksel adc_temp_value + 1
    movf    adc_temp_value + 1, W
    movwf   w16 + 1
    banksel adc_temp_value
    movf    adc_temp_value, W
    movwf   w16
    mulw16f temp_coeff
    rr16fl  w16, 4, 0

    banksel temp_const
    movf    temp_const, W
    addwf   w16, F
    btfsc   STATUS, C
    incf    w16 + 1, F

    movf    w16 + 1, W
    banksel temp_value + 1
    movwf   temp_value + 1
    movf    w16, W
    banksel temp_value
    movwf   temp_value
    endm
    endif

    ifdef section_init
    clr16f  temp_value
    movlw   1 << 4
    selbnkc temp_coeff
    movwf   temp_coeff
    movlw   20
    selbnkc temp_const
    movwf   temp_const
    endif

    ifdef section_idle
    temp_idle
    endif
