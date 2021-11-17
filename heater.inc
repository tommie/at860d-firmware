    ;; Heater I/O module.
    ;;
    ;; RE2, active low

    ifdef module_heater
#undefine module_heater
    endif
#define module_heater

    ifdef section_code
heater_set      macro   file, bit
    btfsc   file, bit
    bcf     PORTE, 2
    btfss   file, bit
    bsf     PORTE, 2
    endm
    endif

    ifdef section_init
    bsf     PORTE, 2
    banksel TRISE
    bcf     TRISE, 2
    banksel PORTE
    endif
