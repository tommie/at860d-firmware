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
    btfss   PORTA, 2
    bcf     file, bit
    btfsc   PORTA, 2
    bsf     file, bit
    endm

swset_get       macro   file, bit
    btfss   PORTB, 1
    bcf     file, bit
    btfsc   PORTB, 1
    bsf     file, bit
    endm

swstandby_get   macro   file, bit
    btfss   PORTA, 4
    bsf     file, bit
    btfsc   PORTA, 4
    bcf     file, bit
    endm

swpower_get macro   file, bit
    btfss   PORTA, 5
    bsf     file, bit
    btfsc   PORTA, 5
    bcf     file, bit
    endm
endif
