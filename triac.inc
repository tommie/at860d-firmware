    ;; Triac driver module.
    ;;
    ;; The heater and airpump are controlled by PWM.
    ;;
    ;; This uses zerocross to determine the phase and frequency.
    ;;
    ;; Note this has a cyclic dependency on timer0. It should be a reverse dependency.
    ;;
    ;; States:
    ;; 0 - Waiting for timer_value to reach zero_cross_time + triac_delay.
    ;; 1 - Enabled optocoupler. Waiting for next timer tick.
    ;; 2 - N/A
    ;; 3 - Disabled optocoupler. Waiting for next zero cross to go to state 0.

    ifdef module_triac
#undefine module_triac
    endif
#define module_triac
    ifndef module_stdlib
    error "stdlib module is a dependency of triac"
    endif
    ifndef module_zerocross
    error "zerocross module is a dependency of triac"
    endif

    ifdef section_udata
triac_delay res 2
triac_state res 1
    endif

    ifdef section_code
    ;; Sets channel `index` to `ratiof` power (file with 0-255).
triac_set macro index, ratiof
    local   set_end

    if ratiof == w16
    error "Cannot use w16 as input for triac_set"
    endif

    ;; if (*ratiof == 0) goto set_end (triac_delay[index] = 0xFF)
    movf    ratiof, F
    movlw   0xFF
    btfsc   STATUS, Z
    goto    set_end

    ;; if (zero_cross_period <= 2) goto set_end (triac_delay[index] = 0xFF)
    movlw   HIGH set_end
    movwf   PCLATH
    movf    zero_cross_period, W
    sublw   2
    movlw   0xFF
    btfsc   STATUS, C
    goto    set_end

    ;; w16 = zero_cross_period * (*ratiof + 1)
    ;; TODO: adjust for sine wave.
    movf    zero_cross_period, W
    movwf   w16
    clrf    w16 + 1
    mulw16f ratiof
    movf    zero_cross_period, W
    addwf   w16, F
    btfsc   STATUS, Z
    incf    w16 + 1, F

    ;; To do nearest-rounding when dividing by 256.
    ;; w16 += 128
    movlw   128
    addwf   w16, F
    btfsc   STATUS, Z
    incf    w16 + 1, F

    ;; W = w16 / 256 <= 2 ? 2 : w16 / 256
    ;; This 2 is based on the accuracy of tmr0, firing of zero_cross_int_next fires (and
    ;; the AC frequency stability).
    movf    w16 + 1, W
    addlw   -2
    btfss   STATUS, C
    clrw
    addlw   2

    ;; triac_delay[index] (...) = zero_cross_period - W
    subwf   zero_cross_period, W

set_end:
    movwf   triac_delay + index
    endm

triac_timer_next macro  index, set_macro
    local       next_end
    local       is_enabled

    if          index >= 2
    error       "Not enough delay variables for triac"
    endif
    if          index >= 4
    error       "Not enough state bits for triac"
    endif

    movlw       HIGH is_enabled
    movwf       PCLATH
    btfsc       triac_state, index * 2
    goto        is_enabled

    ;; if (timer_value - zero_cross_time > triac_delay)
    movlw       HIGH next_end
    movwf       PCLATH
    movf        zero_cross_time, W
    subwf       timer_value, W
    subwf       triac_delay + index, W
    btfsc       STATUS, C
    goto        next_end

    bsf         triac_state, index * 2
    set_macro   triac_state, index * 2
    goto        next_end

is_enabled:
    btfsc       triac_state, index * 2 + 1
    goto        next_end

    ;; This disables after one timer tick (1 ms).
    ;; MOC3023 dv/dt is 10 V/µs at 25 *C. For 240 V, 24 µs is enough.
    ;; (BT139X is even faster than MOC2023.)
    set_macro   triac_state, index * 2 + 1
    bsf         triac_state, index * 2 + 1
    goto        next_end

next_end:
    endm

triac_int_next  macro index, set_macro
    if          index == 0
    clrf        triac_state
    endif
    set_macro   triac_state, 0
    endm
    endif

    ifdef section_init
    movlw   0xFF
    movwf   triac_delay
    movwf   triac_delay + 1
    movwf   triac_state         ; Start in state 3.
    endif
