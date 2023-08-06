    ;; Heater I/O module.
    ;;
    ;; RE2, active low

    ifdef module_heater
#undefine module_heater
    endif
#define module_heater
    ifndef module_adc
    error "adc module is a dependency of heater"
    endif
    ifndef module_stdlib
    error "stdlib module is a dependency of heater"
    endif
    ifndef module_triac_zcc
    error "triac_zcc module is a dependency of heater"
    endif

    ifdef section_udata
heater_value    res 1
    endif

    ifdef section_code
heater_output   macro   file, bit
    banksel PORTE
    movf    PORTE, W
    andlw   ~(1 << 2)
    banksel file
    btfss   file, bit
    iorlw   1 << 2
    banksel PORTE
    movwf   PORTE
    endm

heater_timer_next macro timer_value
    triaczcc_timer_next_ch  0, heater_output, timer_value
    endm

heater_int_next  macro
    triaczcc_int_next_ch    0, heater_output
    endm

heater_setw macro
    banksel heater_value
    movwf   heater_value
    endm

heater_idle macro   true_value
    local       set_triac

    ;; if (true_value > heater_value) W = 0; goto set_triac
    banksel     true_value
    movf        true_value, W
    banksel     heater_value
    subwf       heater_value, W
    movwf       acc1
    movlw       HIGH set_triac
    movwf       PCLATH
    clrw
    btfss       STATUS, C
    goto        set_triac

    ;; W = (heater_value - true_value) / 4
    divp2lf     acc1, 2
    movf        acc1, W

set_triac:
    triaczcc_setw   0
    endm
    endif

    ifdef section_init
    banksel PORTE
    bsf     PORTE, 2
    banksel TRISE
    bcf     TRISE, 2

    banksel heater_value
    clrf    heater_value
    endif

    ifdef section_idle
    heater_idle adc_temp_value
    endif
