    ;; Buzzer I/O module.
    ;;
    ;; RC0

    ifdef module_buzzer
#undefine module_buzzer
    endif
#define module_buzzer

    ifdef section_code
buzzer_set      macro   file, bit
    btfsc   file, bit
    bsf     PORTC, 0
    btfss   file, bit
    bcf     PORTC, 0
    endm
    endif

    ifdef section_init
    bcf     PORTC, 0
    banksel TRISC
    bcf     TRISC, 0
    banksel PORTC
    endif
