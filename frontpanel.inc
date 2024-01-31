    ;; Front panel HCI logic.
    ;;
    ;; State bits
    ;;   0 - Temp displays (0) actual or (1) set. Timed return to 0.
    ;;   1 - Up/down changes (0) temp or (1) airflow.

    ifdef module_frontpanel
#undefine module_frontpanel
    endif
#define module_frontpanel

    ifndef module_buttonpress
    error "buttonpress module is a dependency of frontpanel"
    endif
    ifndef module_display
    error "display module is a dependency of frontpanel"
    endif
    ifndef module_eeprom
    error "eeprom module is a dependency of frontpanel"
    endif
    ifndef module_switch
    error "switch module is a dependency of frontpanel"
    endif
    ifndef module_temp
    error "temp module is a dependency of frontpanel"
    endif
    ifndef module_tempcontrol
    error "tempcontrol module is a dependency of frontpanel"
    endif
    ifndef module_timer0
    error "timer0 module is a dependency of frontpanel"
    endif

    ifdef section_udata
fp_state        res 1
fp_ee_i         res 1
fp_t            res 2

FP_TEMP_SET     EQU 0
FP_PRESET_SET   EQU 1
FP_BTN_AIR      EQU 2
FP_EE_WRITE     EQU 3
FP_RUN_TMR      EQU 4
FP_ARM_TMR      EQU 7
    endif

    ifdef section_eedata
fp_ee_temp  de  0, 0, 0, 15
fp_ee_air   de  31, 0, 0, 255
    endif

    ifdef section_code
fp_display  macro
    local   disp_temp

    temp_get16  w16

    ;; if ((fp_state & 1) == 0) goto disp_temp
    movlw   HIGH disp_temp
    movwf   PCLATH
    selbank fp_state
    btfss   fp_state, FP_TEMP_SET
    goto    disp_temp

    tempc_get16 w16

disp_temp:
    display_set_temp_w16

    movlw   LS2A & LS2D & LS2E & LS2F & LS2DP   ; "*C"
    selbnkc fp_state
    btfsc   fp_state, FP_PRESET_SET
    movlw   LS2A & LS2B & LS2E & LS2F & LS2G    ; "P"
    movwf   display_buf + 6

    clrf    w16 + 1
    selbnkc airpump_value
    movf    airpump_value, W
    movwf   w16
    display_set_air_w16
    endm

fp_idle_preset  macro   btn, index
    local   not_held
    local   done

    movlw   HIGH not_held
    movwf   PCLATH
    buttonpress_skip_if_held btn
    goto    not_held

    ;; Write
    buttonpress_reset_held   btn

    movlw   index
    selbank fp_ee_i
    movwf   fp_ee_i
    selbnkc fp_state
    movlw   (1 << FP_EE_WRITE) | (1 << FP_PRESET_SET) | (1 << FP_ARM_TMR)
    iorwf   fp_state, F

    movlw   HIGH done
    movwf   PCLATH
    goto    done
not_held:

    movlw   HIGH done
    movwf   PCLATH
    buttonpress_skip_if_clicked btn
    goto    done

    ;; Read
    buttonpress_reset_clicked   btn

    eeprom_read_lw  fp_ee_temp + index
    heater_setw

    eeprom_read_lw  fp_ee_air + index
    airpump_setw

    bsf     fp_state, FP_TEMP_SET
    bsf     fp_state, FP_ARM_TMR
done:
    endm

fp_idle macro
    local   up_done
    local   down_done
    local   air_disabled
    local   air_done
    local   arm_done
    local   timer_done
    local   write_ee_done

    movlw   HIGH up_done
    movwf   PCLATH
    buttonpress_skip_if_clicked BTN_UP
    goto    up_done

    buttonpress_reset_clicked   BTN_UP
    fp_change_value_l   1

    selbank fp_state
    bsf     fp_state, FP_ARM_TMR

    movlw   HIGH up_done
    movwf   PCLATH
    btfss   fp_state, FP_BTN_AIR
    bsf     fp_state, FP_TEMP_SET
up_done:

    movlw   HIGH down_done
    movwf   PCLATH
    buttonpress_skip_if_clicked BTN_DOWN
    goto    down_done

    buttonpress_reset_clicked   BTN_DOWN
    fp_change_value_l   -1

    selbnkc fp_state
    bsf     fp_state, FP_ARM_TMR

    movlw   HIGH down_done
    movwf   PCLATH
    btfss   fp_state, FP_BTN_AIR
    bsf     fp_state, FP_TEMP_SET
down_done:

    movlw   HIGH air_done
    movwf   PCLATH
    buttonpress_skip_if_clicked BTN_HEAT_HNDL
    goto    air_done

    buttonpress_reset_clicked   BTN_HEAT_HNDL
    bsf     fp_state, FP_ARM_TMR

    movlw   HIGH air_disabled
    movwf   PCLATH
    selbank fp_state
    btfsc   fp_state, FP_BTN_AIR
    goto    air_disabled

    bsf     fp_state, FP_BTN_AIR
    display_set_cursor  0

    movlw   HIGH air_done
    movwf   PCLATH
    goto    air_done

air_disabled:
    bcf     fp_state, FP_BTN_AIR
    display_clear_cursor
air_done:

    movlw   HIGH arm_done
    movwf   PCLATH
    selbank fp_state
    btfss   fp_state, FP_ARM_TMR
    goto    arm_done

    bcf     fp_state, FP_ARM_TMR
    bsf     fp_state, FP_RUN_TMR

    ;; fp_t = timer_value + 2000
    selbnkc timer_value
    movlw   HIGH 2000
    addwf   timer_value + 1, W
    movwf   fp_t + 1
    movlw   LOW 2000
    addwf   timer_value, W
    movwf   fp_t
    btfsc   STATUS, C
    incf    fp_t + 1, F
arm_done:

    fp_idle_preset  BTN_PRESET_1, 1
    fp_idle_preset  BTN_PRESET_2, 2
    fp_idle_preset  BTN_PRESET_3, 3

    ;; if (!(fp_state & (1 << FP_RUN_TMR))) goto timer_done
    movlw   HIGH timer_done
    movwf   PCLATH
    btfss   fp_state, FP_RUN_TMR
    goto    timer_done

    ;; if (fp_t - timer_value >= 0) goto timer_done
    bge16ff fp_t, timer_value, timer_done

    ;; fp_state &= ~(...)
    selbank fp_state
    movf    fp_state, W
    andlw   ~((1 << FP_TEMP_SET) | (1 << FP_PRESET_SET) | (1 << FP_RUN_TMR))
    iorlw   (1 << FP_EE_WRITE)
    movwf   fp_state
timer_done:

    ;; if (!(fp_state & (1 << FP_EE_WRITE))) goto write_ee_done
    movlw   HIGH write_ee_done
    movwf   PCLATH
    selbank fp_state
    btfss   fp_state, FP_EE_WRITE
    goto    write_ee_done

    fp_try_write_ee tempc_value, fp_ee_temp
    fp_try_write_ee airpump_value, fp_ee_air

    movlw   HIGH write_ee_done
    movwf   PCLATH
    eeprom_skip_if_can_write
    goto    write_ee_done

    ;; Writing both values is done.
    selbnkc fp_state
    bcf     fp_state, FP_EE_WRITE
    selbnkc fp_ee_i
    clrf    fp_ee_i
write_ee_done:
    endm

fp_change_value_l   macro   delta
    local           airpump
    local           exit

    movlw           HIGH airpump
    movwf           PCLATH
    btfsc           fp_state, FP_BTN_AIR
    goto            airpump

    fp_update_lfm   delta, tempc_value, tempc_setw
    movlw           HIGH exit
    movwf           PCLATH
    goto            exit

airpump:
    fp_update_lfm   delta, airpump_value, airpump_setw

exit:
    endm

fp_update_lfm   macro   delta, file, setwmacro
    local   exit

    movlw   HIGH exit
    movwf   PCLATH
    selbank file
    if      delta < 0
    movlw   -(delta)
    subwf   file, W
    btfss   STATUS, C
    else
    movf    file, W
    addlw   delta
    btfsc   STATUS, C
    endif
    goto    exit
    setwmacro

exit:
    endm

fp_try_write_ee macro   file, eeaddr
    local           exit

    movlw           HIGH exit
    movwf           PCLATH
    eeprom_skip_if_can_write
    goto            exit

    movlw           HIGH exit
    movwf           PCLATH
    selbank         fp_ee_i
    movf            fp_ee_i, W
    addlw           eeaddr
    eeprom_read_w
    selbnkc         file
    subwf           file, W
    btfsc           STATUS, Z
    goto            exit

    selbnkc         fp_ee_i
    movf            fp_ee_i, W
    addlw           eeaddr
    selbnkc         EEADR
    movwf           EEADR
    selbnkc         file
    movf            file, W
    eeprom_write_w
exit:
    endm
    endif

    ifdef section_init
    selbank fp_state
    clrf    fp_state
    selbnkc fp_ee_i
    clrf    fp_ee_i

    ;; Initialize from EEPROM.
    eeprom_read_lw  fp_ee_temp
    heater_setw
    eeprom_read_lw  fp_ee_air
    airpump_setw
    endif

    ifdef section_idle
    fp_idle
    endif
