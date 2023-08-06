    ;; Button press logic.
    ;;
    ;; * Click: if release happens after 100 ms and before 500 ms after press.
    ;; * Hold: if release doesn't happen within 800 ms after press. Accelerating repeats.
    ;; * Any number of buttons may be pressed at the same time, but the latest change
    ;;   resets the click and hold timer.

    ifdef module_buttonpress
#undefine module_buttonpress
    endif
#define module_buttonpress

    ifndef module_buttonscan
    error "buttonscan module is a dependency of buttonpress"
    endif
    ifndef module_stdlib
    error "stdlib module is a dependency of buttonpress"
    endif
    ifndef module_timer0
    error "timer0 module is a dependency of buttonpress"
    endif

    ifdef section_udata
BP_DEBOUNCE_MS      equ 100
BP_MAX_CLICK_MS     equ 500
BP_MIN_HOLD_MS      equ 800
BP_MAX_REPEAT_MS    equ 500
BP_MIN_REPEAT_MS    equ 250
BP_REPEAT_STEP_MS   equ 25

    ;; bp_state bits:
BP_VALID_T      equ 0           ; The bp_t value is valid.
BP_DEBOUNCED    equ 1           ; The timer has passed debounce, and is waiting for a not-click tick.
BP_NOT_CLICK    equ 2           ; The timer has passed not-click, and is waiting for a hold.
BP_HELD         equ 3           ; The timer has passed holding, and is waiting for click repeat.

    if BUTTONPRESS_NUM_CHANNELS > 8
    error "BUTTONPRESS_NUM_CHANNELS out of bounds"
    endif

bp_current  res (BUTTONPRESS_NUM_CHANNELS + 7) / 8
bp_prev     res (BUTTONPRESS_NUM_CHANNELS + 7) / 8
bp_clicked  res (BUTTONPRESS_NUM_CHANNELS + 7) / 8
bp_holding  res (BUTTONPRESS_NUM_CHANNELS + 7) / 8
bp_t        res 2
bp_ival     res 2
bp_state    res 1
    endif

    ifdef section_code
buttonpress_skip_if_clicked macro   index
    banksel bp_clicked + (index / 8)
    btfss   bp_clicked + (index / 8), index % 8
    endm

buttonpress_reset_clicked   macro   index
    banksel bp_clicked + (index / 8)
    bcf     bp_clicked + (index / 8), index % 8
    endm

bp_idle     macro
    local   done_set_clicked
    local   no_diff
    local   not_clicked
    local   start_hold
    local   repeat_click
    local   done_cap_ival
    local   idle_done

    ;; IOR the two HEAT buttons. They're equivalent, and we don't need
    ;; the high byte of buttonscan_current.
    banksel buttonscan_current
    movf    buttonscan_current, W
    btfsc   buttonscan_current + (BTN_HEAT_PANEL / 8), BTN_HEAT_PANEL % 8
    iorlw   1 << BTN_HEAT_HNDL
    banksel bp_current
    movwf   bp_current

    ;; if (bp_current == bp_prev) goto no_diff
    movlw   HIGH no_diff
    movwf   PCLATH
    movf    bp_current, W
    banksel bp_prev
    xorwf   bp_prev, W
    btfsc   STATUS, Z
    goto    no_diff

    ;; if ((bp_state & (1 << BP_DEBOUNCED)) && !(bp_state & (1 << BP_NOT_CLICK))) bp_clicked = bp_prev & ~bp_current
    movlw   HIGH done_set_clicked
    movwf   PCLATH
    banksel bp_state
    movf    bp_state, W
    andlw   (1 << BP_DEBOUNCED) | (1 << BP_NOT_CLICK)
    xorlw   (1 << BP_DEBOUNCED)
    btfss   STATUS, Z
    goto    done_set_clicked

    banksel bp_current
    movf    bp_current, W
    xorlw   0xFF
    banksel bp_prev
    andwf   bp_prev, W
    banksel bp_clicked
    movwf   bp_clicked
done_set_clicked:

    ;; bp_holding &= bp_current
    banksel bp_current
    movf    bp_current, W
    banksel bp_holding
    andwf   bp_holding, F

    ;; bp_t = timer_value + BP_DEBOUNCE_MS
    banksel timer_value
    movf    timer_value, W
    banksel bp_t
    movwf   bp_t
    banksel timer_value + 1
    movf    timer_value + 1, W
    banksel bp_t + 1
    movwf   bp_t + 1
    movlw   LOW BP_DEBOUNCE_MS
    banksel bp_t
    addwf   bp_t, F
    banksel bp_t + 1
    btfsc   STATUS, C
    incf    bp_t + 1, F
    movlw   HIGH BP_DEBOUNCE_MS
    addwf   bp_t + 1, F

    ;; bp_state = (1 << BP_VALID_T)
    movlw   0
    banksel bp_current
    movf    bp_current, F
    btfss   STATUS, Z
    movlw   (1 << BP_VALID_T)
    banksel bp_state
    movwf   bp_state

    movlw   HIGH idle_done
    movwf   PCLATH
    goto    idle_done

no_diff:
    ;; if (!(bp_state & (1 << BP_VALID_T))) goto idle_done
    movlw   HIGH idle_done
    movwf   PCLATH
    banksel bp_state
    btfss   bp_state, BP_VALID_T
    goto    idle_done

    ;; if (bp_t - timer_value >= 0) goto idle_done
    bge16ff bp_t, timer_value, idle_done

    ;; bp_state &= ~(1 << BP_VALID_T)
    banksel bp_state
    bcf     bp_state, BP_VALID_T

    ;; if (bp_state & (1 << BP_HELD)) goto repeat_click
    movlw   HIGH repeat_click
    movwf   PCLATH
    banksel bp_state
    btfsc   bp_state, BP_HELD
    goto    repeat_click

    ;; if (bp_state & (1 << BP_NOT_CLICK)) goto start_hold
    movlw   HIGH start_hold
    movwf   PCLATH
    banksel bp_state
    btfsc   bp_state, BP_NOT_CLICK
    goto    start_hold

    ;; if (bp_state & (1 << BP_DEBOUNCED)) goto not_clicked
    movlw   HIGH not_clicked
    movwf   PCLATH
    btfsc   bp_state, BP_DEBOUNCED
    goto    not_clicked

    ;; bp_t += BP_MAX_CLICK_MS - BP_DEBOUNCE_MS
    movlw   LOW (BP_MAX_CLICK_MS - BP_DEBOUNCE_MS)
    banksel bp_t
    addwf   bp_t, F
    banksel bp_t + 1
    btfsc   STATUS, C
    incf    bp_t + 1, F
    movlw   HIGH (BP_MAX_CLICK_MS - BP_DEBOUNCE_MS)
    addwf   bp_t + 1, F

    ;; bp_state |= (1 << BP_DEBOUNCED) | BP_VALID_T
    movlw   (1 << BP_DEBOUNCED) | (1 << BP_VALID_T)
    banksel bp_state
    iorwf   bp_state, F

    movlw   HIGH idle_done
    movwf   PCLATH
    goto    idle_done

not_clicked:
    ;; bp_state |= 1 << BP_NOT_CLICK
    banksel bp_state
    bsf     bp_state, BP_NOT_CLICK

    ;; bp_t += BP_MIN_HOLD_MS - BP_MAX_CLICK_MS
    movlw   LOW (BP_MIN_HOLD_MS - BP_MAX_CLICK_MS)
    banksel bp_t
    addwf   bp_t, F
    banksel bp_t + 1
    btfsc   STATUS, C
    incf    bp_t + 1, F
    movlw   HIGH (BP_MIN_HOLD_MS - BP_MAX_CLICK_MS)
    addwf   bp_t + 1, F

    ;; bp_state |= 1 << BP_VALID_T
    banksel bp_state
    bsf     bp_state, BP_VALID_T

    movlw   HIGH idle_done
    movwf   PCLATH
    goto    idle_done

start_hold:
    ;; bp_state |= 1 << BP_HELD
    banksel bp_state
    bsf     bp_state, BP_HELD

    banksel bp_current
    movf    bp_current, W
    banksel bp_holding
    movwf   bp_holding

    ;; bp_ival = BP_MAX_REPEAT_MS
    movlw   LOW BP_MAX_REPEAT_MS
    banksel bp_ival
    movwf   bp_ival
    movlw   HIGH BP_MAX_REPEAT_MS
    banksel bp_ival + 1
    movwf   bp_ival + 1

    ;; fallthrough

repeat_click:
    ;; bp_clicked |= bp_holding & BUTTONPRESS_REPEAT_MASK
    banksel bp_holding
    movf    bp_holding, W
    andlw   BUTTONPRESS_REPEAT_MASK
    banksel bp_clicked
    iorwf   bp_clicked, F

    ;; bp_t += bp_ival
    banksel bp_ival
    movf    bp_ival, W
    banksel bp_t
    addwf   bp_t, F
    banksel bp_t + 1
    btfsc   STATUS, C
    incf    bp_t + 1, F
    movf    bp_ival + 1, W
    addwf   bp_t + 1, F

    ;; bp_state |= 1 << BP_VALID_T
    banksel bp_state
    bsf     bp_state, BP_VALID_T

    ;; bp_ival -= BP_REPEAT_STEP_MS
    movlw   LOW BP_REPEAT_STEP_MS
    banksel bp_ival
    subwf   bp_ival, F
    banksel bp_ival + 1
    btfss   STATUS, C
    decf    bp_ival + 1, F
    movlw   HIGH BP_REPEAT_STEP_MS
    subwf   bp_ival + 1, F

    ;; if (bp_ival < BP_MIN_REPEAT_MS) bp_ival = BP_MIN_REPEAT_MS
    bge16lf bp_ival, BP_MIN_REPEAT_MS, done_cap_ival

    movlw   LOW BP_MIN_REPEAT_MS
    banksel bp_ival
    movwf   bp_ival
    movlw   HIGH BP_MIN_REPEAT_MS
    banksel bp_ival + 1
    movwf   bp_ival + 1
done_cap_ival:

    movlw   HIGH idle_done
    movwf   PCLATH
    goto    idle_done

idle_done:
    banksel bp_current
    movf    bp_current, W
    banksel bp_prev
    movwf   bp_prev
    endm
    endif

    ifdef section_init
    banksel bp_t
    clrf    bp_t
    banksel bp_t + 1
    clrf    bp_t + 1
    banksel bp_ival
    clrf    bp_ival
    banksel bp_ival + 1
    clrf    bp_ival + 1
    banksel bp_state
    clrf    bp_state
i   set     0
    while   i < (BUTTONPRESS_NUM_CHANNELS + 7) / 8
    banksel bp_current + i
    clrf    bp_current + i
    banksel bp_prev + i
    clrf    bp_prev + i
    banksel bp_clicked + i
    clrf    bp_clicked + i
    banksel bp_holding + i
    clrf    bp_holding + i
i   set     i + 1
    endw
    endif

    ifdef section_idle
    bp_idle
    endif
