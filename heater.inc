    ;; Heater I/O module.
    ;;
    ;; RE2, active low

    ifdef module_heater
#undefine module_heater
    endif
#define module_heater
    ifndef module_triac_zcc
    error "triac_zcc module is a dependency of heater"
    endif

    ifdef section_code
heater_output   macro   file, bit
    btfsc   file, bit
    bcf     PORTE, 2
    btfss   file, bit
    bsf     PORTE, 2
    endm

heater_timer_next macro timer_value
    triaczcc_timer_next_ch  0, heater_output, timer_value
    endm

heater_int_next  macro
    triaczcc_int_next_ch    0, heater_output
    endm

heater_setw macro
    triaczcc_setw   0
    endm
    endif

    ifdef section_init
    bsf     PORTE, 2
    banksel TRISE
    bcf     TRISE, 2
    banksel PORTE
    endif
