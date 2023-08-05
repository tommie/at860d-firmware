    ;; Front panel HCI logic.
    ;;
    ;; State bits
    ;;   0 - Temp displays (0) actual or (1) set. Timed return to 0.
    ;;   1 - Up/down changes (0) temp or (1) airflow.

    ifdef module_frontpanel
#undefine module_frontpanel
    endif
#define module_frontpanel

    ifndef module_adc
    error "adc module is a dependency of frontpanel"
    endif
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
    ifndef module_timer0
    error "timer0 module is a dependency of frontpanel"
    endif

    ifdef section_udata
fp_state        res 1
fp_t            res 2
fp_ee_t         res 2

FP_TEMP_SET     EQU 0
FP_BTN_AIR      EQU 1
FP_TEMP_DIRTY   EQU 2
FP_AIR_DIRTY    EQU 3
FP_EE_WRITE     EQU 4
FP_RUN_TMR      EQU 5
FP_ARM_TMR      EQU 7
    endif

    ifdef section_eedata
fp_ee_temp  de  0, 0, 0, 15
fp_ee_air   de  31, 0, 0, 255
    endif

    ifdef section_code
fp_display  macro
    local   disp_temp

    banksel adc_temp_value
    movf    adc_temp_value + 1, W
    movwf   w16 + 1
    movf    adc_temp_value, W
    movwf   w16

    ;; if ((fp_state & 1) == 0) goto disp_temp
    movlw   HIGH disp_temp
    movwf   PCLATH
    banksel fp_state
    btfss   fp_state, FP_TEMP_SET
    goto    disp_temp

    clrf    w16 + 1
    banksel heater_value
    movf    heater_value, W
    movwf   w16

disp_temp:
    display_set_temp_w16

    movlw   LS2A & LS2D & LS2E & LS2F & LS2DP   ; "*C"
    movwf   display_buf + 6

    clrf    w16 + 1
    banksel airpump_value
    movf    airpump_value, W
    movwf   w16
    display_set_air_w16
    endm

fp_idle_preset  macro   btn, i
    local   done

    movlw   HIGH done
    movwf   PCLATH
    buttonpress_skip_if_clicked btn
    goto    done

    buttonpress_reset_clicked   btn

    eeprom_read_lw  fp_ee_temp + i
    heater_setw

    eeprom_read_lw  fp_ee_air + i
    airpump_setw
done:
    endm

fp_idle macro
    local   up_done
    local   down_done
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

    banksel fp_state
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

    banksel fp_state
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
    movlw   1 << FP_BTN_AIR
    banksel fp_state
    xorwf   fp_state, F
air_done:

    movlw   HIGH arm_done
    movwf   PCLATH
    banksel fp_state
    btfss   fp_state, FP_ARM_TMR
    goto    arm_done

    bcf     fp_state, FP_ARM_TMR
    bsf     fp_state, FP_RUN_TMR

    ;; fp_t = timer_value + 2000
    banksel timer_value
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
    banksel fp_state
    movf    fp_state, W
    andlw   ~((1 << FP_TEMP_SET) | (1 << FP_RUN_TMR))
    iorlw   (1 << FP_EE_WRITE)
    movwf   fp_state
timer_done:

    ;; if (!(fp_state & (1 << FP_EE_WRITE))) goto write_ee_done
    movlw   HIGH write_ee_done
    movwf   PCLATH
    banksel fp_state
    btfss   fp_state, FP_EE_WRITE
    goto    write_ee_done

    fp_try_write_ee FP_TEMP_DIRTY, heater_value, fp_ee_temp
    fp_try_write_ee FP_AIR_DIRTY, airpump_value, fp_ee_air

    movlw   HIGH write_ee_done
    movwf   PCLATH
    eeprom_skip_if_can_write
    goto    write_ee_done

    banksel fp_state
    bcf     fp_state, FP_EE_WRITE
write_ee_done:
    endm

fp_change_value_l   macro   delta
    local           airpump
    local           exit

    movlw           HIGH airpump
    movwf           PCLATH
    btfsc           fp_state, FP_BTN_AIR
    goto            airpump

    fp_update_lfm   delta, heater_value, heater_setw
    banksel         fp_state
    bsf             fp_state, FP_TEMP_DIRTY
    movlw           HIGH exit
    movwf           PCLATH
    goto            exit

airpump:
    fp_update_lfm   delta, airpump_value, airpump_setw
    banksel         fp_state
    bsf             fp_state, FP_AIR_DIRTY

exit:
    endm

fp_update_lfm   macro   delta, file, setwmacro
    local   exit

    movlw   HIGH exit
    movwf   PCLATH
    banksel file
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

fp_try_write_ee macro   bit, file, eeaddr
    local           exit

    movlw           HIGH exit
    movwf           PCLATH
    banksel         fp_state
    btfss           fp_state, bit
    goto            exit

    eeprom_skip_if_can_write
    goto            exit

    banksel         fp_state
    bcf             fp_state, bit

    eeprom_read_lw  eeaddr
    banksel         file
    subwf           file, W
    btfsc           STATUS, Z
    goto            exit

    movf            file, W
    eeprom_write_lw eeaddr
exit:
    endm
    endif

    ifdef section_init
    banksel fp_state
    clrf    fp_state

    ;; Initialize from EEPROM.
    eeprom_read_lw  fp_ee_temp
    heater_setw
    eeprom_read_lw  fp_ee_air
    airpump_setw
    endif

    ifdef section_idle
    fp_idle
    endif
