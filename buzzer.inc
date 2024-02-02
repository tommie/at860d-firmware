    ;; Buzzer I/O module.
    ;;
    ;; RC0

    ifdef module_buzzer
#undefine module_buzzer
    endif
#define module_buzzer

    ifdef section_code
buzzer_set      macro   file, bit
    selbank PORTC
    movf    PORTC, W
    andlw   ~(1 << 0)
    selbnkc file
    btfss   file, bit
    iorlw   1 << 0
    selbnkc PORTC
    movwf   PORTC
    endm
    endif

    ifdef section_init
    selbank PORTC
    bcf     PORTC, 0
    selbnkc TRISC
    bcf     TRISC, 0
    endif
