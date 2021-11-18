    ;; Triac phase-fired control module.
    ;;
    ;; The airpump is controlled by PFC (not power factor correction).
    ;;
    ;; This uses zerocross to determine the phase and frequency.
    ;;
    ;; States:
    ;; 0 - Waiting for timer_value to reach zero_cross_time + triac_delay.
    ;; 1 - Enabled optocoupler. Waiting for next timer tick.
    ;; 2 - N/A
    ;; 3 - Disabled optocoupler. Waiting for next zero cross to go to state 0.

    ifdef module_triac_pfc
#undefine module_triac_pfc
    endif
#define module_triac_pfc
    ifndef module_stdlib
    error "stdlib module is a dependency of triac_pfc"
    endif
    ifndef module_zerocross
    error "zerocross module is a dependency of triac_pfc"
    endif

    ifdef section_udata
    ifndef TRIACPFC_NUM_CHANNELS
    error "TRIACPFC_NUM_CHANNELS is not defined"
    endif
triacpfc_delay res TRIACPFC_NUM_CHANNELS
triacpfc_state res (TRIACPFC_NUM_CHANNELS + 3) / 4
    endif

    ifdef section_code
    ;; Sets channel `channel` to `ratiof` power (file with 0-255).
triacpfc_set macro channel, ratiof
    local   set_end

    if channel >= TRIACPFC_NUM_CHANNELS
    error "triac_pfc channel out of bounds"
    endif
    if ratiof == w16
    error "Cannot use w16 as input for triacpfc_set"
    endif

    ;; if (*ratiof == 0) goto set_end (triacpfc_delay[channel] = 0xFF)
    movf    ratiof, F
    movlw   0xFF
    btfsc   STATUS, Z
    goto    set_end

    ;; if (zero_cross_period <= 2) goto set_end (triacpfc_delay[channel] = 0xFF)
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

    ;; triacpfc_delay[channel] (...) = zero_cross_period - W
    subwf   zero_cross_period, W

set_end:
    movwf   triacpfc_delay + channel
    endm

triacpfc_timer_next_ch macro  channel, set_macro, timer_value
    local       next_end
    local       is_enabled

    if channel >= TRIACPFC_NUM_CHANNELS
    error "triac_pfc channel out of bounds"
    endif

    movlw       HIGH is_enabled
    movwf       PCLATH
    btfsc       triacpfc_state + (channel / 4), (channel % 4) * 2
    goto        is_enabled

    ;; if (timer_value - zero_cross_time <= triacpfc_delay[channel]) goto next_end
    movlw       HIGH next_end
    movwf       PCLATH
    movf        zero_cross_time, W
    subwf       timer_value, W
    subwf       triacpfc_delay + channel, W
    btfsc       STATUS, C
    goto        next_end

    bsf         triacpfc_state + (channel / 4), (channel % 4) * 2
    set_macro   triacpfc_state + (channel / 4), (channel % 4) * 2
    goto        next_end

is_enabled:
    btfsc       triacpfc_state + (channel / 4), (channel % 4) * 2 + 1
    goto        next_end

    ;; This disables after one timer tick (1 ms).
    ;; MOC3023 dv/dt is 10 V/µs at 25 *C. For 240 V, 24 µs is enough.
    ;; (BT139X is even faster than MOC2023.)
    set_macro   triacpfc_state + (channel / 4), (channel % 4) * 2 + 1
    bsf         triacpfc_state + (channel / 4), (channel % 4) * 2 + 1
    goto        next_end

next_end:
    endm

triacpfc_int_next_ch  macro channel, set_macro
    if channel >= TRIACPFC_NUM_CHANNELS
    error "triac_pfc channel out of bounds"
    endif

    clrf        triacpfc_state + (channel / 4)
    set_macro   triacpfc_state + (channel / 4), (channel % 4) * 2
    endm

triacpfc_init   macro
    movlw   0xFF
i   set     0
    while   i < TRIACPFC_NUM_CHANNELS
    movwf   triacpfc_delay + i
i   set     i + 1
    endw
i   set     0
    while   i < TRIACPFC_NUM_CHANNELS / 4
    movwf   triacpfc_state + (i / 4)        ; Start in state 3.
i   set     i + 1
    endw
    endm
    endif

    ifdef section_init
    triacpfc_init
    endif