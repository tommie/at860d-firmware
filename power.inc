    ;; Power enable I/O module.
    ;;
    ;; This activates a triac to bypass the main switch, allowing the
	;; MCU to let the airpump run until the temperature is low enough.
    ;; It's not in the triac module, because the control is just on/off.
    ;;
    ;; RE0, active low

    ifdef module_power
#undefine module_power
    endif
#define module_power

    ifdef section_code
power_set       macro   file, bit
    btfsc   file, bit
    bcf     PORTE, 0
    btfss   file, bit
    bsf     PORTE, 0
    endm
    endif

    ifdef section_init
    bsf     PORTE, 0
    banksel TRISE
    bcf     TRISE, 0
    banksel PORTE
    endif
