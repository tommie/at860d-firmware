    ;; Buzzer I/O module.
    ;;
    ;; RC0

    ifdef module_buzzer
#undefine module_buzzer
    endif
#define module_buzzer

    ifdef section_code
buzzer_set      macro   file, bit
    banksel PORTC
    movf    PORTC, W
    andlw   ~(1 << 0)
    banksel file
    btfss   file, bit
    iorlw   1 << 0
    banksel PORTC
    movwf   PORTC
    endm
    endif

    ifdef section_init
    banksel PORTC
    bcf     PORTC, 0
    banksel TRISC
    bcf     TRISC, 0
    endif
