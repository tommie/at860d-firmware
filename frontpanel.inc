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
    ifndef module_fault
    error "fault module is a dependency of frontpanel"
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
FP_KNOB_TEMP    equ 0
FP_KNOB_AIR     equ 1

    ;; fp_state bits
FP_TEMP_SET     equ 0
FP_PRESET_SET   equ 1
FP_BTN_AIR      equ 2
FP_RUN_TMR      equ 3
FP_ARM_TMR      equ 7

fp_state        res 1
fp_t            res 2
    endif

    ifdef section_code
fp_idle_preset  macro   btn, index
    local   not_held
    local   done

    pagesel not_held
    buttonpress_skip_if_held btn
    goto    not_held

    ;; Write
    buttonpress_reset_held  btn
    uc_store                index
    selbank                 fp_state
    movlw                   (1 << FP_PRESET_SET) | (1 << FP_ARM_TMR)
    iorwf                   fp_state, F

    pagesel done
    goto    done
not_held:

    pagesel done
    buttonpress_skip_if_clicked btn
    goto    done

    ;; Read
    buttonpress_reset_clicked   btn
    knob_deactivate_all
    uc_load                     index

    selbank fp_state
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

    knob_idle   FP_KNOB_TEMP, 2, TEMPCONTROL_MIN_TEMP, TEMPCONTROL_MAX_TEMP, tempc_value, fp_tempc_set16
    knob_idle   FP_KNOB_AIR, 1, 0, AIRPUMP_MAX_RATIO, airpump_value, fp_airpump_setw

    pagesel up_done
    buttonpress_skip_if_clicked BTN_UP
    goto    up_done

    buttonpress_reset_clicked   BTN_UP
    fp_change_value_l   1

    selbank fp_state
    bsf     fp_state, FP_ARM_TMR
    btfss   fp_state, FP_BTN_AIR
    bsf     fp_state, FP_TEMP_SET
up_done:

    pagesel down_done
    buttonpress_skip_if_clicked BTN_DOWN
    goto    down_done

    buttonpress_reset_clicked   BTN_DOWN
    fp_change_value_l   -1

    selbank fp_state
    bsf     fp_state, FP_ARM_TMR
    btfss   fp_state, FP_BTN_AIR
    bsf     fp_state, FP_TEMP_SET
down_done:

    pagesel air_done
    buttonpress_skip_if_clicked BTN_HEAT_HNDL
    goto    air_done

    buttonpress_reset_clicked   BTN_HEAT_HNDL
    selbank fp_state
    bsf     fp_state, FP_ARM_TMR

    pagesel air_disabled
    movlw   1 << FP_BTN_AIR
    selbank fp_state
    xorwf   fp_state, F
    btfss   fp_state, FP_BTN_AIR
    goto    air_disabled

    knob_select         FP_KNOB_AIR
    display_set_cursor  0

    pagesel air_done
    goto    air_done

air_disabled:
    knob_select FP_KNOB_TEMP
    display_clear_cursor
air_done:

    pagesel arm_done
    selbank fp_state
    btfss   fp_state, FP_ARM_TMR
    goto    arm_done

    bcf     fp_state, FP_ARM_TMR
    bsf     fp_state, FP_RUN_TMR

    ;; fp_t = timer_value + FRONTPANEL_SET_DELAY
    selbank timer_value
    movlw   HIGH FRONTPANEL_SET_DELAY
    selbank timer_value + 1
    addwf   timer_value + 1, W
    selbank fp_t + 1
    movwf   fp_t + 1
    movlw   LOW FRONTPANEL_SET_DELAY
    selbank timer_value
    addwf   timer_value, W
    selbank fp_t
    movwf   fp_t
    selbank fp_t + 1
    btfsc   STATUS, C
    incf    fp_t + 1, F

    ;; if (fp_state.temp_set) display_set_cursor(3)
    selbank fp_state
    btfss   fp_state, FP_TEMP_SET
    goto    arm_done

    pagesel temp_cursor_done
    btfsc   fp_state, FP_BTN_AIR
    goto    temp_cursor_done

    knob_select         FP_KNOB_TEMP
    display_set_cursor  3
temp_cursor_done:
arm_done:

    fp_idle_preset  BTN_PRESET_1, 1
    fp_idle_preset  BTN_PRESET_2, 2
    fp_idle_preset  BTN_PRESET_3, 3

    ;; if (!(fp_state & (1 << FP_RUN_TMR))) goto timer_done
    pagesel timer_done
    selbank fp_state
    btfss   fp_state, FP_RUN_TMR
    goto    timer_done

    ;; if (fp_t - timer_value > 0) goto timer_done
    gts16ff fp_t, timer_value
    pagesel timer_done
    btfsc   STATUS, C
    goto    timer_done

    ;; fp_state &= ~(...)
    selbank fp_state
    movf    fp_state, W
    andlw   ~((1 << FP_TEMP_SET) | (1 << FP_PRESET_SET) | (1 << FP_RUN_TMR))
    movwf   fp_state

    pagesel clear_cursor_done
    selbank fp_state
    btfsc   fp_state, FP_BTN_AIR
    goto    clear_cursor_done

    knob_select FP_KNOB_TEMP
    display_clear_cursor
clear_cursor_done:

    ;; Start storing the current value.
    ifndef  TEMPC_DIRECT_CONTROL
    uc_store    0
    endif
timer_done:
done:
    endm

fp_change_value_l   macro   delta
    local           airpump
    local           exit

    pagesel         airpump
    selbank         fp_state
    btfsc           fp_state, FP_BTN_AIR
    goto            airpump

    fp_update_lfm16 2*(delta), tempc_value, tempc_set16, FP_KNOB_TEMP ; FP9.1
    pagesel         exit
    goto            exit

airpump:
    fp_update_lfm   delta, airpump_value, airpump_setw, FP_KNOB_AIR

exit:
    endm

    ;; Applies a signed delta to file, using set16macro to make the update.
    ;; Saturates at either end.
fp_update_lfm   macro   delta, file, setwmacro, knob
    local   exit

    pagesel exit
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
    knob_deactivate knob

exit:
    endm

    ;; Applies a signed delta to file, using set16macro to make the update.
    ;; Saturates at either end.
fp_update_lfm16 macro   delta, file, set16macro, knob
    local       exit

    pagesel     exit
    mov16ff     file, w16
    add16lf     delta, w16
    btfsc       w16 + 1, 7
    goto        exit

    set16macro  w16
    knob_deactivate knob

exit:
    endm

fp_airpump_setw macro
    airpump_setw
    selbank     fp_state
    bsf         fp_state, FP_ARM_TMR
    endm

fp_tempc_set16  macro   src
    movlw       ~1
    selbank     src
    andwf       src, F          ; Truncate to integer.
    tempc_set16 src

    selbank     fp_state
    bsf         fp_state, FP_ARM_TMR
    bsf         fp_state, FP_TEMP_SET
    endm

fp_display_fault macro
    local   temp_sensor_done
    local   mend

    selbank display_buf + 6
    movlw   0xFF
    movwf   display_buf + 6

    pagesel temp_sensor_done
    fault_skip_if_fault FAULT_TEMP_SENSOR
    goto    temp_sensor_done

    movlw   LSA & LSC & LSD & LSF & LSG ; "S"
    selbank display_buf + 5
    movwf   display_buf + 5
    movlw   LSG                         ; "-"
    selbank display_buf + 4
    movwf   display_buf + 4
    movlw   LSA & LSD & LSE & LSF & LSG ; "E"
    selbank display_buf + 3
    movwf   display_buf + 3

    pagesel mend
    goto    mend
temp_sensor_done:

    pagesel mend
    fault_skip_if_fault FAULT_HEATER
    goto    mend

    movlw   LSB & LSC & LSE & LSF & LSG ; "H"
    selbank display_buf + 5
    movwf   display_buf + 5
    movlw   LSG                         ; "-"
    selbank display_buf + 4
    movwf   display_buf + 4
    movlw   LSA & LSD & LSE & LSF & LSG ; "E"
    selbank display_buf + 3
    movwf   display_buf + 3
mend:
    endm

fp_display  macro
    local   no_fault
    local   disp_temp
    local   disp_temp_unit
    local   temp_done
    local   disp_air
    local   air_done
    local   done

    mov16ff tempc_value, w16

    ;; if (fp_state.temp_set) goto disp_temp
    pagesel disp_temp
    selbank fp_state
    btfsc   fp_state, FP_TEMP_SET
    goto    disp_temp

    ifndef  DISABLE_FAULT
    ;; if (fault_is_good) goto no_fault
    pagesel no_fault
    fault_is_good
    btfsc   STATUS, Z
    goto    no_fault

    fp_display_fault

    pagesel temp_done
    goto    temp_done
    endif   ; DISABLE_FAULT

no_fault:
    temp_get16  w16

    ;; if (w16 < MIN_TEMP) display_buf[5:3] = "Low"
    ge16lf  w16, FRONTPANEL_MIN_TEMP
    btfsc   STATUS, C
    goto    disp_temp

    movlw   LSD & LSE & LSF       ; "L"
    selbank display_buf + 5
    movwf   display_buf + 5
    movlw   LSC & LSD & LSE & LSG ; "o"
    selbank display_buf + 4
    movwf   display_buf + 4
    movlw   LSC & LSD & LSE       ; "w"
    selbank display_buf + 3
    movwf   display_buf + 3

    pagesel disp_temp_unit
    goto    disp_temp_unit

disp_temp:
    rr16fl  w16, 1, 0           ; To FP9.0
    display_set_temp_w16

disp_temp_unit:
    movlw   LS2A & LS2D & LS2E & LS2F & LS2DP   ; "*C"
    selbank fp_state
    btfsc   fp_state, FP_PRESET_SET
    movlw   LS2A & LS2B & LS2E & LS2F & LS2G    ; "P"
    selbank display_buf + 6
    movwf   display_buf + 6
temp_done:

    clrf    w16 + 1
    selbank airpump_value
    movf    airpump_value, W
    movwf   w16

    ;; if (airpump_value != 0) goto disp_air
    pagesel disp_air
    iorlw   0
    btfss   STATUS, Z
    goto    disp_air

    ;; Show "OFF"
    movlw   LSA & LSB & LSC & LSD & LSE & LSF ; "O"
    selbank display_buf + 2
    movwf   display_buf + 2
    movlw   LSA & LSE & LSF & LSG ; "F"
    selbank display_buf + 1
    movwf   display_buf + 1
    selbank display_buf + 0
    movwf   display_buf + 0
    display_cur_update

    pagesel air_done
    goto    air_done

disp_air:
    display_set_air_w16
air_done:
done:
    endm

fp_idle_cooldown macro   endlabel
    local       mend

    ifndef      DISABLE_COOLDOWN
    pagesel     mend
    cooldown_skip_if_active
    goto        mend

    buttonpress_reset_all

    temp_get16  w16
    rr16fl      w16, 1, 0       ; To FP9.0
    display_set_temp_w16
    movlw       LS2A & LS2D & LS2E & LS2F & LS2DP   ; "*C"
    selbank     display_buf + 6
    movwf       display_buf + 6

    movlw       LSA & LSD & LSE & LSF               ; "C"
    selbank     display_buf + 2
    movwf       display_buf + 2
    movlw       LSA & LSB & LSC & LSD & LSE & LSF   ; "O"
    selbank     display_buf + 1
    movwf       display_buf + 1
    movlw       LSD & LSE & LSF                     ; "L"
    selbank     display_buf + 0
    movwf       display_buf + 0

    pagesel     endlabel
    goto        endlabel
    endif
mend:
    endm

fp_idle_selftest macro   endlabel
    local       failed
    local       mend

    ifndef      DISABLE_SELFTEST
    pagesel     mend
    selftest_skip_if_not_passed
    goto        mend

    buttonpress_reset_all

    movlw       LSA & LSE & LSF             ; "T"
    selbank     display_buf + 5
    movwf       display_buf + 5
    selbank     display_buf + 3
    movwf       display_buf + 3
    movlw       LSA & LSC & LSD & LSF & LSG ; "S"
    selbank     display_buf + 4
    movwf       display_buf + 4

    selbank     selftest_state
    movf        selftest_state, W
    movwf       w16
    selbank     selftest_state + 1
    movf        selftest_state + 1, W
    andlw       0x03
    movwf       w16 + 1
    display_set_air_w16

    pagesel     failed
    selftest_skip_if_not_failed
    goto        failed

    movlw       ~0
    selbank     display_buf + 6
    movwf       display_buf + 6

    pagesel     endlabel
    goto        endlabel

failed:
    movlw       LS2A & LS2E & LS2F & LS2G ; "F"
    selbank     display_buf + 6
    movwf       display_buf + 6

    pagesel     endlabel
    goto        endlabel
    endif
mend:
    endm

fp_idle_standby  macro   endlabel
    local       mend

    ifndef      DISABLE_STANDBY
    pagesel     mend
    standby_skip_if_sleep
    goto        mend

    buttonpress_reset_all

    temp_get16  w16
    rr16fl      w16, 1, 0       ; To FP9.0
    display_set_temp_w16
    movlw       LS2A & LS2D & LS2E & LS2F & LS2DP   ; "*C"
    selbank     display_buf + 6
    movwf       display_buf + 6

    movlw       LSA & LSC & LSD & LSF & LSG ; "S"
    selbank     display_buf + 2
    movwf       display_buf + 2
    movlw       LSD & LSE & LSF             ; "L"
    selbank     display_buf + 1
    movwf       display_buf + 1
    movlw       LSA & LSB & LSE & LSF & LSG ; "P"
    selbank     display_buf + 0
    movwf       display_buf + 0

    pagesel     endlabel
    goto        endlabel
    endif
mend:
    endm
    endif

    ifdef section_init
    selbank         fp_state
    clrf            fp_state
    knob_select     FP_KNOB_TEMP
    endif

    ifdef section_idle
    fp_idle
    endif
