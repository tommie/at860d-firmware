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
    banksel file
    movf    file, W
    andlw   ~(1 << bit)
    banksel PORTA
    btfss   PORTA, 2
    iorlw   1 << bit
    banksel file
    movwf   file
    endm

swset_get       macro   file, bit
    banksel file
    movf    file, W
    andlw   ~(1 << bit)
    banksel PORTB
    btfss   PORTB, 1
    iorlw   1 << bit
    banksel file
    movwf   file
    endm

swstandby_get   macro   file, bit
    banksel file
    movf    file, W
    andlw   ~(1 << bit)
    banksel PORTA
    btfss   PORTA, 4
    iorlw   1 << bit
    banksel file
    movwf   file
    endm

swpower_get macro   file, bit
    banksel file
    movf    file, W
    andlw   ~(1 << bit)
    banksel PORTA
    btfss   PORTA, 5
    iorlw   1 << bit
    banksel file
    movwf   file
    endm
endif
