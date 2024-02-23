    ;; Power enable I/O module.
    ;;
    ;; This activates a triac to bypass the main switch, allowing the
	;; MCU to let the airpump run until the temperature is low enough. ;
    ;;
    ;; RA2 - Set/compensation mode photodiode, active low
    ;; RA4 - Standby reed relay in handle, active low
    ;; RA5 - Power switch state, active low
    ;; RB1 - Calibration mode photodiode, active low

    ifdef module_switch
#undefine module_switch
    endif
#define module_switch

ifdef section_code
swcal_skip_if_clear macro
    selbank PORTB
    btfss   PORTB, 1
    endm

swset_skip_if_set   macro
    selbank PORTA
    btfsc   PORTA, 2
    endm

swset_skip_if_clear macro
    selbank PORTA
    btfss   PORTA, 2
    endm

swstandby_skip_if_clear macro
    selbank PORTA
    btfss   PORTA, 4
    endm

swpower_skip_if_clear   macro
    selbank PORTA
    btfss   PORTA, 5
    endm
endif
