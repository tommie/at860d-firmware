    ;; Buzzer I/O module.
    ;;
    ;; RC0

    ifdef module_buzzer
#undefine module_buzzer
    endif
#define module_buzzer

    ifdef section_code
buzzer_set      macro   file, bit
    clrw
    banksel file
    btfsc   file, bit
    movlw   1 << 0
    banksel PORTC
    iorwf   PORTC, F
    endm
    endif

    ifdef section_init
    banksel PORTC
    bcf     PORTC, 0
    banksel TRISC
    bcf     TRISC, 0
    endif
