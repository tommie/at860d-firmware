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
BP_DEBOUNCE_MS      equ 50
BP_MAX_CLICK_MS     equ 500
BP_MIN_HOLD_MS      equ 3000
BP_MAX_REPEAT_MS    equ 300
BP_MIN_REPEAT_MS    equ 50
BP_REPEAT_STEP_MS   equ 50

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
bp_held     res (BUTTONPRESS_NUM_CHANNELS + 7) / 8
bp_holding  res (BUTTONPRESS_NUM_CHANNELS + 7) / 8
bp_t        res 2               ; In units of 500 µs.
bp_ival     res 2               ; In units of 500 µs.
bp_state    res 1
    endif

    ifdef section_code
buttonpress_skip_if_clicked macro   index
    selbank bp_clicked + (index / 8)
    btfss   bp_clicked + (index / 8), index % 8
    endm

buttonpress_reset_clicked   macro   index
    selbank bp_clicked + (index / 8)
    bcf     bp_clicked + (index / 8), index % 8
    endm

buttonpress_skip_if_held macro   index
    selbank bp_held + (index / 8)
    btfss   bp_held + (index / 8), index % 8
    endm

buttonpress_reset_held macro   index
    selbank bp_held + (index / 8)
    bcf     bp_held + (index / 8), index % 8
    endm

    ;; Resets all clicked and held bits.
buttonpress_reset_all macro
    selbank bp_clicked
i   set     0
    while   i < (BUTTONPRESS_NUM_CHANNELS + 7) / 8
    selbank bp_clicked + i
    clrf    bp_clicked + i
    selbank bp_held + i
    clrf    bp_held + i
i   set     i + 1
    endw
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
    selbank buttonscan_current
    movf    buttonscan_current, W
    selbank buttonscan_current + (BTN_HEAT_PANEL / 8)
    btfsc   buttonscan_current + (BTN_HEAT_PANEL / 8), BTN_HEAT_PANEL % 8
    iorlw   1 << BTN_HEAT_HNDL
    selbank bp_current
    movwf   bp_current

    ;; if (bp_current == bp_prev) goto no_diff
    pagesel no_diff
    movf    bp_current, W
    selbank bp_prev
    xorwf   bp_prev, W
    btfsc   STATUS, Z
    goto    no_diff

    ;; if ((bp_state & (1 << BP_DEBOUNCED)) && !(bp_state & (1 << BP_NOT_CLICK))) bp_clicked = bp_prev & ~bp_current
    pagesel done_set_clicked
    selbank bp_state
    movf    bp_state, W
    andlw   (1 << BP_DEBOUNCED) | (1 << BP_NOT_CLICK)
    xorlw   (1 << BP_DEBOUNCED)
    btfss   STATUS, Z
    goto    done_set_clicked

    selbank bp_current
    movf    bp_current, W
    xorlw   0xFF
    selbank bp_prev
    andwf   bp_prev, W
    selbank bp_clicked
    movwf   bp_clicked
done_set_clicked:

    ;; bp_holding &= bp_current
    selbank bp_current
    movf    bp_current, W
    selbank bp_holding
    andwf   bp_holding, F

    ;; bp_t = timer_value + BP_DEBOUNCE_MS
    mov16ff timer_value, bp_t
    add16lf (BP_DEBOUNCE_MS) * 2, bp_t

    ;; bp_state = (1 << BP_VALID_T)
    movlw   0
    selbank bp_current
    movf    bp_current, F
    btfss   STATUS, Z
    movlw   (1 << BP_VALID_T)
    selbank bp_state
    movwf   bp_state

    pagesel idle_done
    goto    idle_done

no_diff:
    ;; if (!(bp_state & (1 << BP_VALID_T))) goto idle_done
    pagesel idle_done
    selbank bp_state
    btfss   bp_state, BP_VALID_T
    goto    idle_done

    ;; if (bp_t >= timer_value) goto idle_done
    pagesel idle_done
    gts16ff timer_value, bp_t
    btfss   STATUS, C
    goto    idle_done

    ;; bp_state &= ~(1 << BP_VALID_T)
    selbank bp_state
    bcf     bp_state, BP_VALID_T

    ;; if (bp_state & (1 << BP_HELD)) goto repeat_click
    pagesel repeat_click
    selbank bp_state
    btfsc   bp_state, BP_HELD
    goto    repeat_click

    ;; if (bp_state & (1 << BP_NOT_CLICK)) goto start_hold
    pagesel start_hold
    btfsc   bp_state, BP_NOT_CLICK
    goto    start_hold

    ;; if (bp_state & (1 << BP_DEBOUNCED)) goto not_clicked
    pagesel not_clicked
    btfsc   bp_state, BP_DEBOUNCED
    goto    not_clicked

    ;; bp_t += BP_MAX_CLICK_MS - BP_DEBOUNCE_MS
    add16lf (BP_MAX_CLICK_MS - BP_DEBOUNCE_MS) * 2, bp_t

    ;; bp_state |= (1 << BP_DEBOUNCED) | BP_VALID_T
    movlw   (1 << BP_DEBOUNCED) | (1 << BP_VALID_T)
    selbank bp_state
    iorwf   bp_state, F

    pagesel idle_done
    goto    idle_done

not_clicked:
    ;; bp_state |= 1 << BP_NOT_CLICK
    selbank bp_state
    bsf     bp_state, BP_NOT_CLICK

    ;; if (bp_current & REPEAT_MASK) goto start_hold
    pagesel start_hold
    selbank bp_current
    movf    bp_current, W
    andlw   BUTTONPRESS_REPEAT_MASK
    btfss   STATUS, Z
    goto    start_hold

    ;; bp_t += BP_MIN_HOLD_MS - BP_MAX_CLICK_MS
    add16lf (BP_MIN_HOLD_MS - BP_MAX_CLICK_MS) * 2, bp_t

    ;; bp_state |= 1 << BP_VALID_T
    selbank bp_state
    bsf     bp_state, BP_VALID_T

    pagesel idle_done
    goto    idle_done

start_hold:
    ;; bp_state |= 1 << BP_HELD
    selbank bp_state
    bsf     bp_state, BP_HELD

    selbank bp_current
    movf    bp_current, W
    selbank bp_holding
    movwf   bp_holding
    selbank bp_held
    iorwf   bp_held, F

    ;; bp_ival = BP_MAX_REPEAT_MS
    mov16lf (BP_MAX_REPEAT_MS) * 2, bp_ival

    ;; fallthrough

repeat_click:
    ;; bp_clicked |= bp_holding & BUTTONPRESS_REPEAT_MASK
    selbank bp_holding
    movf    bp_holding, W
    andlw   BUTTONPRESS_REPEAT_MASK
    selbank bp_clicked
    iorwf   bp_clicked, F

    ;; bp_t += bp_ival
    add16ff bp_ival, bp_t

    ;; bp_state |= 1 << BP_VALID_T
    selbank bp_state
    bsf     bp_state, BP_VALID_T

    ;; bp_ival -= BP_REPEAT_STEP_MS
    add16lf -(BP_REPEAT_STEP_MS) * 2, bp_ival

    ;; if (bp_ival < BP_MIN_REPEAT_MS) bp_ival = BP_MIN_REPEAT_MS
    ge16lf  bp_ival, (BP_MIN_REPEAT_MS) * 2
    pagesel done_cap_ival
    btfsc   STATUS, C
    goto    done_cap_ival

    mov16lf (BP_MIN_REPEAT_MS) * 2, bp_ival
done_cap_ival:

    pagesel idle_done
    goto    idle_done

idle_done:
    selbank bp_current
    movf    bp_current, W
    selbank bp_prev
    movwf   bp_prev
    endm

bp_init     macro
    local   i

    clr16f  bp_t
    clr16f  bp_ival
    selbank bp_state
    clrf    bp_state
i   set     0
    while   i < (BUTTONPRESS_NUM_CHANNELS + 7) / 8
    selbank bp_current + i
    clrf    bp_current + i
    selbank bp_prev + i
    clrf    bp_prev + i
    selbank bp_clicked + i
    clrf    bp_clicked + i
    selbank bp_held + i
    clrf    bp_held + i
    selbank bp_holding + i
    clrf    bp_holding + i
i   set     i + 1
    endw
    endm
    endif

    ifdef section_init
    bp_init
    endif

    ifdef section_idle
    bp_idle
    endif
