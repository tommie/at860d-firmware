    ;; Front panel HCI logic.
    ;;
    ;; State bits
    ;;   0 - Temp displays (0) actual or (1) set. Timed return to 0.
    ;;   1 - Up/down changes (0) temp or (1) airflow.

    ifdef module_frontpanel
#undefine module_frontpanel
    endif
#define module_frontpanel

    ifndef module_airpump
    error "airpump module is a dependency of frontpanel"
    endif
    ifndef module_buttonpress
    error "buttonpress module is a dependency of frontpanel"
    endif
    ifndef module_display
    error "display module is a dependency of frontpanel"
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
    ifndef module_userconfig
    error "userconfig module is a dependency of frontpanel"
    endif

    ifdef section_udata
fp_state        res 1
fp_t            res 2

FP_TEMP_SET     EQU 0
FP_PRESET_SET   EQU 1
FP_BTN_AIR      EQU 2
FP_RUN_TMR      EQU 3
FP_ARM_TMR      EQU 7
    endif

    ifdef section_code
fp_idle_preset  macro   btn, index
    local   not_held
    local   done

    movlw   HIGH not_held
    movwf   PCLATH
    buttonpress_skip_if_held btn
    goto    not_held

    ;; Write
    buttonpress_reset_held  btn
    uc_store                index
    selbnkc                 fp_state
    movlw                   (1 << FP_PRESET_SET) | (1 << FP_ARM_TMR)
    iorwf                   fp_state, F

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
    uc_load                     index

    bsf     fp_state, FP_TEMP_SET
    bsf     fp_state, FP_ARM_TMR
done:
    endm

fp_idle macro
    ifndef  FRONTPANEL_SET_DELAY
    error   "FRONTPANEL_SET_DELAY is not defined"
    endif

    local   up_done
    local   down_done
    local   air_disabled
    local   air_done
    local   temp_cursor_done
    local   arm_done
    local   clear_cursor_done
    local   timer_done
    local   done

    ;; Modes of operation. In priority order.
    fp_idle_cooldown done
    fp_idle_selftest done
    fp_idle_standby  done

    fp_display

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

    ;; fp_t = timer_value + FRONTPANEL_SET_DELAY
    selbnkc timer_value
    movlw   HIGH FRONTPANEL_SET_DELAY
    addwf   timer_value + 1, W
    movwf   fp_t + 1
    movlw   LOW FRONTPANEL_SET_DELAY
    addwf   timer_value, W
    movwf   fp_t
    btfsc   STATUS, C
    incf    fp_t + 1, F

    ;; if (fp_state.temp_set) display_set_cursor(3)
    selbnkc fp_state
    btfss   fp_state, FP_TEMP_SET
    goto    arm_done

    movlw   HIGH temp_cursor_done
    movwf   PCLATH
    selbnkc fp_state
    btfsc   fp_state, FP_BTN_AIR
    goto    temp_cursor_done

    display_set_cursor  3
temp_cursor_done:
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
    movwf   fp_state

    movlw   HIGH clear_cursor_done
    movwf   PCLATH
    selbnkc fp_state
    btfsc   fp_state, FP_BTN_AIR
    goto    clear_cursor_done

    display_clear_cursor
clear_cursor_done:

    ;; Start storing the current value.
    uc_store    0
timer_done:
done:
    endm

fp_change_value_l   macro   delta
    local           airpump
    local           exit

    movlw           HIGH airpump
    movwf           PCLATH
    btfsc           fp_state, FP_BTN_AIR
    goto            airpump

    fp_update_lfm16 2*(delta), tempc_value, tempc_set16 ; FP9.1
    movlw           HIGH exit
    movwf           PCLATH
    goto            exit

airpump:
    fp_update_lfm   delta, airpump_value, airpump_setw

exit:
    endm

    ;; Applies a signed delta to file, using set16macro to make the update.
    ;; Saturates at either end.
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

    ;; Applies a signed delta to file, using set16macro to make the update.
    ;; Saturates at either end.
fp_update_lfm16 macro   delta, file, set16macro
    local       exit

    movlw       HIGH exit
    movwf       PCLATH
    mov16ff     file, w16
    add16lf     delta, w16
    if          delta < 0
    btfss       STATUS, C
    else
    btfsc       STATUS, C
    endif
    goto        exit
    set16macro  w16

exit:
    endm

fp_display  macro
    local   disp_temp
    local   disp_temp_unit
    local   done

    mov16ff tempc_value, w16

    ;; if (fp_state.temp_set) goto disp_temp
    movlw   HIGH disp_temp
    movwf   PCLATH
    selbank fp_state
    btfsc   fp_state, FP_TEMP_SET
    goto    disp_temp

    temp_get16  w16

    ;; if (w16 == TEMP_CONST_INIT) display_buf = "Low"
    ge16lf  w16, TEMP_CONST_INIT
    btfss   STATUS, Z
    goto    disp_temp

    movlw   LSD & LSE & LSF       ; "L"
    selbnkc display_buf + 5
    movwf   display_buf + 5
    movlw   LSC & LSD & LSE & LSG ; "o"
    selbnkc display_buf + 4
    movwf   display_buf + 4
    movlw   LSC & LSD & LSE       ; "w"
    selbnkc display_buf + 3
    movwf   display_buf + 3

    movlw   HIGH disp_temp_unit
    movwf   PCLATH
    goto    disp_temp_unit

disp_temp:
    rr16fl  w16, 1, 0       ; To FP9.0
    display_set_temp_w16

disp_temp_unit:
    movlw   LS2A & LS2D & LS2E & LS2F & LS2DP   ; "*C"
    selbank fp_state
    btfsc   fp_state, FP_PRESET_SET
    movlw   LS2A & LS2B & LS2E & LS2F & LS2G    ; "P"
    selbnkc display_buf + 6
    movwf   display_buf + 6

    clrf    w16 + 1
    selbnkc airpump_value
    movf    airpump_value, W
    movwf   w16
    display_set_air_w16
done:
    endm

fp_idle_cooldown macro   endlabel
    local       mend

    ifndef      DISABLE_COOLDOWN
    movlw       HIGH mend
    movwf       PCLATH
    cooldown_skip_if_active
    goto        mend

    buttonpress_reset_all

    temp_get16  w16
    rr16fl      w16, 1, 0       ; To FP9.0
    display_set_temp_w16
    movlw       LS2A & LS2D & LS2E & LS2F & LS2DP   ; "*C"
    selbnkc     display_buf + 6
    movwf       display_buf + 6

    movlw       LSA & LSD & LSE & LSF               ; "C"
    selbnkc     display_buf + 2
    movwf       display_buf + 2
    movlw       LSA & LSB & LSC & LSD & LSE & LSF   ; "O"
    selbnkc     display_buf + 1
    movwf       display_buf + 1
    movlw       LSD & LSE & LSF                     ; "L"
    selbnkc     display_buf + 0
    movwf       display_buf + 0

    movlw       HIGH endlabel
    movwf       PCLATH
    goto        endlabel
    endif
mend:
    endm

fp_idle_selftest macro   endlabel
    local       failed
    local       mend

    ifndef      DISABLE_SELFTEST
    movlw       HIGH mend
    movwf       PCLATH
    selftest_skip_if_not_passed
    goto        mend

    buttonpress_reset_all

    movlw       LSA & LSE & LSF             ; "T"
    selbnkc     display_buf + 5
    movwf       display_buf + 5
    selbnkc     display_buf + 3
    movwf       display_buf + 3
    movlw       LSA & LSC & LSD & LSF & LSG ; "S"
    selbnkc     display_buf + 4
    movwf       display_buf + 4

    selbnkc     selftest_state
    movf        selftest_state, W
    movwf       w16
    selbnkc     selftest_state + 1
    movf        selftest_state + 1, W
    andlw       0x03
    movwf       w16 + 1
    display_set_air_w16

    movlw       HIGH failed
    movwf       PCLATH
    selftest_skip_if_not_failed
    goto        failed

    movlw       ~0
    selbnkc     display_buf + 6
    movwf       display_buf + 6

    movlw       HIGH endlabel
    movwf       PCLATH
    goto        endlabel

failed:
    movlw       LS2A & LS2E & LS2F & LS2G ; "F"
    selbank     display_buf + 6
    movwf       display_buf + 6

    movlw       HIGH endlabel
    movwf       PCLATH
    goto        endlabel
    endif
mend:
    endm

fp_idle_standby  macro   endlabel
    local       mend

    ifndef      DISABLE_STANDBY
    movlw       HIGH mend
    movwf       PCLATH
    standby_skip_if_active
    goto        mend

    buttonpress_reset_all

    temp_get16  w16
    rr16fl      w16, 1, 0       ; To FP9.0
    display_set_temp_w16
    movlw       LS2A & LS2D & LS2E & LS2F & LS2DP   ; "*C"
    selbank     display_buf + 6
    movwf       display_buf + 6

    movlw       LSA & LSC & LSD & LSF & LSG ; "S"
    selbnkc     display_buf + 2
    movwf       display_buf + 2
    movlw       LSD & LSE & LSF             ; "L"
    selbnkc     display_buf + 1
    movwf       display_buf + 1
    movlw       LSA & LSB & LSE & LSF & LSG ; "P"
    selbnkc     display_buf + 0
    movwf       display_buf + 0

    movlw       HIGH endlabel
    movwf       PCLATH
    goto        endlabel
    endif
mend:
    endm
    endif

    ifdef section_init
    selbank fp_state
    clrf    fp_state
    endif

    ifdef section_idle
    fp_idle
    endif
