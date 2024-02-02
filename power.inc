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
power_set   macro   file, bit
    movinvbit       file, bit, PORTE, 0
    endm
    endif

    ifdef section_init
    selbank PORTE
    bsf     PORTE, 0
    selbnkc TRISE
    bcf     TRISE, 0
    endif
