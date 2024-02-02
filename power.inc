    ;; Power bypass I/O module.
    ;;
    ;; This activates a triac to bypass the mains switch, allowing the
    ;; MCU to let the airpump run until the temperature is low enough.
    ;;
    ;; RE0, active low

    ifdef module_power
#undefine module_power
    endif
#define module_power

    ifdef section_code
power_set       macro   file, bit
    selbank PORTE
    movf    PORTE, W
    andlw   ~(1 << 0)
    selbnkc file
    btfss   file, bit
    iorlw   1 << 0
    selbnkc PORTE
    movwf   PORTE
    endm
    endif

    ifdef section_init
    selbank PORTE
    bsf     PORTE, 0
    selbnkc TRISE
    bcf     TRISE, 0
    endif
