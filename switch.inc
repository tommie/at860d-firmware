    ;; Power enable I/O module.
    ;;
    ;; This activates a triac to bypass the main switch, allowing the
	;; MCU to let the airpump run until the temperature is low enough. ;
    ;;
    ;; RA2 - Calibration photodiode
    ;; RA4 - Standby reed relay in handle, active low
    ;; RA5 - Power switch state, active low
    ;; RB1 - Set (calibration) photodiode

    ifdef module_switch
#undefine module_switch
    endif
#define module_switch

ifdef section_code
swcal_get       macro   file, bit
    clrw
    banksel PORTA
    btfsc   PORTA, 2
    movlw   1 << bit
    banksel file
    iorwf   file, F
    endm

swset_get       macro   file, bit
    clrw
    banksel PORTB
    btfsc   PORTB, 1
    movlw   1 << bit
    banksel file
    iorwf   file, F
    endm

swstandby_get   macro   file, bit
    clrw
    banksel PORTA
    btfss   PORTA, 4
    movlw   1 << bit
    banksel file
    iorwf   file, F
    endm

swpower_get macro   file, bit
    clrw
    banksel PORTA
    btfss   PORTA, 5
    movlw   1 << bit
    banksel file
    iorwf   file, F
    endm
endif
