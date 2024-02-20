    ;; Buzzer I/O module.
    ;;
    ;; RC0

    ifdef module_buzzer
#undefine module_buzzer
    endif
#define module_buzzer

    ifdef section_code
buzzer_set  macro   file, bit
    movbit          file, bit, PORTC, 0
    endm
    endif

    ifdef section_init
    selbank PORTC
    bcf     PORTC, 0
    selbank TRISC
    bcf     TRISC, 0
    endif
