    ;; Calibration mode.
    ;;
    ;; This mode is activated by booting with the "multi-function card" inserted
    ;; "CAL" up. It is not like the original firmware's mode.
    ;;
    ;; * Button P1 selects the temperature coefficient mode, modifying the value
    ;;   using up/down buttons. "AT1" is displayed under "Air."
    ;; * Button P2 selects the temperature offset mode, modifying the value
    ;;   using up/down buttons. "AT2" is displayed under "Air."
    ;; * Selection button selects the set-temperature mode, where the air
    ;;   temperature is modified using up/down buttons. "CAL" is dispalyed
    ;;   under "Air."
    ;; * Holding the selection button commits the new values to persistent
    ;;   storage.
    ;;
    ;; The airpump is always at maximum.

    ifdef module_calibration
#undefine module_calibration
    endif
#define module_calibration

    ifndef module_buttonpress
    error "buttonpress module is a dependency of calibration"
    endif
    ifndef module_display
    error "display module is a dependency of calibration"
    endif
    ifndef module_eeprom
    error "eeprom module is a dependency of calibration"
    endif
    ifndef module_switch
    error "switch module is a dependency of calibration"
    endif
    ifndef module_temp
    error "temp module is a dependency of calibration"
    endif
    ifndef module_timer0
    error "timer0 module is a dependency of calibration"
    endif
    ifndef module_userconfig
    error "userconfig module is a dependency of calibration"
    endif

    ifdef section_udata
CAL_ACK_DURATION    equ 2000 / 128 ; In 128 ms units.

    ;; cal_state bits
CAL_ACTIVE      equ 0
CAL_COEFF       equ 1
CAL_CONST       equ 2
CAL_EE_WRITE    equ 3
CAL_EE_ACK      equ 4

cal_state       res 1
cal_t           res 1              ; In 128 ms units.
    endif ; section_udata

    ifdef section_eedata
cal_ee_temp_coeff   de      TEMP_COEFF_INIT
cal_ee_temp_const   de      TEMP_CONST_INIT
    endif ; section_eedata

    ifdef section_code
cal_skip_if_active  macro
    selbank cal_state
    btfss   cal_state, CAL_ACTIVE
    endm

cal_skip_if_not_active  macro
    selbank cal_state
    btfsc   cal_state, CAL_ACTIVE
    endm

cal_display macro
    local   no_fault
    local   not_coeff
    local   not_const
    local   mend

    ifndef  DISABLE_FAULT
    ;; if (fault_is_good) goto no_fault
    pagesel no_fault
    fault_is_good
    btfsc   STATUS, Z
    goto    no_fault

    movlw   LSA & LSD & LSE & LSF & LSG ; "E"
    selbank display_buf + 5
    movwf   display_buf + 5
    movlw   LSE & LSG                   ; "r"
    selbank display_buf + 4
    movwf   display_buf + 4
    selbank display_buf + 3
    movwf   display_buf + 3

    pagesel mend
    goto    mend
    endif   ; DISABLE_FAULT
no_fault:

    ;; if (cal_state.ee_ack) goto mend
    pagesel mend
    selbank cal_state
    btfsc   cal_state, CAL_EE_ACK
    goto    mend

    pagesel not_coeff
    selbank cal_state
    btfss   cal_state, CAL_COEFF
    goto    not_coeff

    temp_get16  w16
    rr16fl      w16, 1, 0       ; To FP9.0
    display_set_temp_w16

    pagesel mend
    goto    mend
not_coeff:

    pagesel not_const
    selbank cal_state
    btfss   cal_state, CAL_CONST
    goto    not_const

    temp_get16  w16
    rr16fl      w16, 1, 0       ; To FP9.0
    display_set_temp_w16

    pagesel mend
    goto    mend
not_const:

    mov16ff tempc_value, w16
    rr16fl  w16, 1, 0           ; To FP9.0
    display_set_temp_w16

mend:
    endm

cal_updatew macro
    local   not_coeff
    local   not_const
    local   mend

    pagesel not_coeff
    selbank cal_state
    btfss   cal_state, CAL_COEFF
    goto    not_coeff

    selbank temp_coeff
    addwf   temp_coeff, F

    pagesel mend
    goto    mend
not_coeff:

    pagesel not_const
    btfss   cal_state, CAL_CONST
    goto    not_const

    selbank temp_const
    addwf   temp_const, F

    pagesel mend
    goto    mend
not_const:

    ;; tempc_set16(tempc_value + 2*W)
    movwf       w16
    addwf       w16, F          ; To FP 9.1.
    selbank     tempc_value + 1
    clrw
    btfsc       w16, 7
    xorlw       0xFF
    movwf       w16 + 1
    movf        tempc_value + 1, W
    addwf       w16 + 1, F
    selbank     tempc_value
    movf        tempc_value, W
    addwf       w16, F
    btfsc       STATUS, C
    incf        w16 + 1, F
    tempc_set16 w16
mend:
    endm

cal_buttons macro
    local   up_done
    local   down_done
    local   p1_done
    local   p2_done
    local   heat_done
    local   heat_held_done

    pagesel up_done
    buttonpress_skip_if_clicked BTN_UP
    goto    up_done

    buttonpress_reset_clicked   BTN_UP
    movlw   1
    cal_updatew
up_done:

    pagesel down_done
    buttonpress_skip_if_clicked BTN_DOWN
    goto    down_done

    buttonpress_reset_clicked   BTN_DOWN
    movlw   -1
    cal_updatew
down_done:

    pagesel p1_done
    buttonpress_skip_if_clicked BTN_PRESET_1
    goto    p1_done

    buttonpress_reset_clicked   BTN_PRESET_1
    selbank cal_state
    bsf     cal_state, CAL_COEFF
    bcf     cal_state, CAL_CONST

    cal_display_update_mode
p1_done:

    pagesel p2_done
    buttonpress_skip_if_clicked BTN_PRESET_2
    goto    p2_done

    buttonpress_reset_clicked   BTN_PRESET_2
    selbank cal_state
    bcf     cal_state, CAL_COEFF
    bsf     cal_state, CAL_CONST

    cal_display_update_mode
p2_done:

    pagesel heat_done
    buttonpress_skip_if_clicked BTN_HEAT_HNDL
    goto    heat_done

    buttonpress_reset_clicked   BTN_HEAT_HNDL
    selbank cal_state
    bcf     cal_state, CAL_COEFF
    bcf     cal_state, CAL_CONST

    cal_display_update_mode
heat_done:

    pagesel heat_held_done
    buttonpress_skip_if_held    BTN_HEAT_HNDL
    goto    heat_held_done

    buttonpress_reset_held      BTN_HEAT_HNDL
    selbank cal_state
    bsf     cal_state, CAL_EE_WRITE
heat_held_done:
    endm

cal_eeprom  macro
    local   ack_done
    local   writing_done
    local   mend

    ;; if (!cal_state.ee_ack) goto ack_done
    pagesel ack_done
    selbank cal_state
    btfss   cal_state, CAL_EE_ACK
    goto    ack_done

    ;; if ((int) timer_value/256 - cal_t < 0) goto ack_done
    selbank cal_t
    movf    cal_t, W
    selbank timer_value + 1
    subwf   timer_value + 1, W
    andlw   0x80
    btfss   STATUS, Z
    goto    ack_done

    selbank cal_state
    bcf     cal_state, CAL_EE_ACK

    cal_display_update_mode
ack_done:

    ;; if (!cal_state.ee_write) goto mend
    pagesel mend
    selbank cal_state
    btfss   cal_state, CAL_EE_WRITE
    goto    mend

    ifndef  DISABLE_FAULT
    ;; if (!fault_is_good) goto writing_done
    pagesel writing_done
    fault_is_good
    btfss   STATUS, Z
    goto    writing_done
    endif   ; DISABLE_FAULT

    eeprom_try_write_fl temp_coeff, cal_ee_temp_coeff, 0, 0
    eeprom_try_write_fl temp_const, cal_ee_temp_const, 0, 0
    pagesel             mend
    eeprom_skip_if_can_write
    goto                mend

writing_done:
    ;; Writing all values is done.
    selbank cal_state
    bcf     cal_state, CAL_EE_WRITE
    bsf     cal_state, CAL_EE_ACK

    ;; cal_t = timer_value/256 + ACK_DURATION
    selbank timer_value + 1
    movf    timer_value + 1, W
    addlw   CAL_ACK_DURATION
    selbank cal_t
    movwf   cal_t

    cal_display_update_mode
mend:
    endm

cal_idle    macro
    cal_display
    cal_buttons
    cal_eeprom
    endm

cal_display_update_mode macro
    pagesel cal_display_update_mode_func
    call    cal_display_update_mode_func
    endm

cal_display_update_mode_    macro
    local   not_ack
    local   not_coeff
    local   not_const
    local   mend

    pagesel not_ack
    selbank cal_state
    btfss   cal_state, CAL_EE_ACK
    goto    not_ack

    movlw   LSA & LSC & LSD & LSF & LSG ; "S"
    selbank display_buf + 2
    movwf   display_buf + 2
    movlw   LSA & LSD & LSE & LSF & LSG ; "E"
    selbank display_buf + 1
    movwf   display_buf + 1
    movlw   LSA & LSE & LSF             ; "T"
    selbank display_buf + 0
    movwf   display_buf + 0

    pagesel mend
    goto    mend
not_ack:

    pagesel not_coeff
    btfss   cal_state, CAL_COEFF
    goto    not_coeff

    movlw   LSA & LSB & LSC & LSE & LSF & LSG ; "A"
    selbank display_buf + 2
    movwf   display_buf + 2
    movlw   LSA & LSE & LSF                   ; "T"
    selbank display_buf + 1
    movwf   display_buf + 1
    movlw   LSB & LSC                         ; "1"
    selbank display_buf + 0
    movwf   display_buf + 0

    pagesel mend
    goto    mend
not_coeff:

    pagesel not_const
    btfss   cal_state, CAL_CONST
    goto    not_const

    movlw   LSA & LSB & LSC & LSE & LSF & LSG ; "A"
    selbank display_buf + 2
    movwf   display_buf + 2
    movlw   LSA & LSE & LSF                   ; "T"
    selbank display_buf + 1
    movwf   display_buf + 1
    movlw   LSA & LSB & LSD & LSE & LSG       ; "2"
    selbank display_buf + 0
    movwf   display_buf + 0

    pagesel mend
    goto    mend
not_const:

    movlw   LSA & LSD & LSE & LSF             ; "C"
    selbank display_buf + 2
    movwf   display_buf + 2
    movlw   LSA & LSB & LSC & LSE & LSF & LSG ; "A"
    selbank display_buf + 1
    movwf   display_buf + 1
    movlw   LSD & LSE & LSF                   ; "L"
    selbank display_buf + 0
    movwf   display_buf + 0

mend:
    endm

cal_display_update_mode_func:
    cal_display_update_mode_
    return

cal_init    macro
    local           not_active

    clrw
    swcal_skip_if_clear
    movlw           1 << CAL_ACTIVE
    selbank         cal_state
    movwf           cal_state
    pagesel         not_active
    btfss           cal_state, CAL_ACTIVE
    goto            not_active

    movlw           AIRPUMP_MAX_RATIO
    airpump_setw

    cal_display_update_mode
    display_set_cursor 3
not_active:

    eeprom_read_lw  cal_ee_temp_coeff
    selbank         temp_coeff
    movwf           temp_coeff
    eeprom_read_lw  cal_ee_temp_const
    selbank         temp_const
    movwf           temp_const
    endm
    endif ; section_code

    ifdef section_init
    cal_init
    endif ; section_init
