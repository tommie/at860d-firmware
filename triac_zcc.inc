    ;; Triac zero-cross control module.
    ;;
    ;; The heater and power bypass are controlled by ZCC.
    ;;
    ;; This uses zerocross to determine when to fire a trigger burst.
    ;;
    ;; For an on-ratio of R:
    ;;   Delay between on-pulses: 1/R
    ;;   In fixed-point that is: 2^N / (R*2^N)
    ;;   We precompute it for each on-ratio. Empirically, for N=5, the precomputed value requires 3 additional fractional bits to provide a well-spread pattern.
    ;;   Each cycle, we add the precomputed value to "next_time", resetting every N cycles.
    ;;
    ;; States:
    ;; 0 - Disabled output. Waiting for zero crossings until count is zero.
    ;; 1 - Enabled output. Waiting for next timer tick.

    ifdef module_triac_zcc
#undefine module_triac_zcc
    endif
#define module_triac_zcc
    ifndef module_stdlib
    error "stdlib module is a dependency of triac_zcc"
    endif
    ifndef module_zerocross
    error "zerocross module is a dependency of triac_zcc"
    endif

    ifdef section_udata
    ifndef TRIACZCC_NUM_CHANNELS
    error "TRIACZCC_NUM_CHANNELS is not defined"
    endif
triaczcc_cycle      res 1
triaczcc_delay      res TRIACZCC_NUM_CHANNELS
triaczcc_prev_on    res TRIACZCC_NUM_CHANNELS
triaczcc_state      res (TRIACZCC_NUM_CHANNELS + 7) / 8
    endif

    ifdef section_code
    ifndef TRIACZCC_NUM_FRAC_BITS
    error "TRIACZCC_NUM_FRAC_BITS is not defined"
    endif

    ;; Sets channel `channel` to `ratiof` power (file with 0-255).
triaczcc_setw macro channel
    local       i
    if channel >= TRIACZCC_NUM_CHANNELS
    error "triac_zcc channel out of bounds"
    endif

    ;; Round up, but clamp at 255.
    addlw       (1 << (8 - TRIACZCC_NUM_FRAC_BITS)) - 1
    btfsc       STATUS, C
    movlw       0xFF

    movwf       acc1
i   set         0
    while       i < 8 - TRIACZCC_NUM_FRAC_BITS
    bcf         STATUS, C
    rrf         acc1, F
i   set         i + 1
    endw

    pagesel     triaczcc_table
    movf        acc1, W
    call        triaczcc_table
    selbank     triaczcc_delay + channel
    movwf       triaczcc_delay + channel
    endm

    ;; Sets Z to 1 if the channel is off.
triaczcc_is_inactive    macro   channel
    selbank     triaczcc_delay + channel
    movf        triaczcc_delay + channel, F
    endm

triaczcc_int_next  macro
    if TRIACZCC_NUM_FRAC_BITS + TRIACZCC_NUM_EXTRA_BITS > 8
    error "triaczcc_cycle too small for TRIACZCC_NUM_FRAC_BITS+TRIACZCC_NUM_EXTRA_BITS"
    endif

    movlw       1 << TRIACZCC_NUM_EXTRA_BITS
    selbank     triaczcc_cycle
    addwf       triaczcc_cycle, F
    endm

triaczcc_int_next_ch  macro channel, set_macro
    if channel >= TRIACZCC_NUM_CHANNELS
    error "triac_zcc channel out of bounds"
    endif

    local       next_end

    pagesel     next_end
    selbank     triaczcc_state + (channel / 8)
    btfsc       triaczcc_state + (channel / 8), (channel % 8)
    goto        next_end

    ;; if (triaczcc_delay == 0) goto next_end
    selbnkc     triaczcc_delay + channel
    movf        triaczcc_delay + channel, F
    btfsc       STATUS, Z
    goto        next_end

    ;; if (triaczcc_cycle - triaczcc_prev_on[channel] <= triaczcc_delay[channel]) goto next_end
    selbnkc     triaczcc_prev_on + channel
    movf        triaczcc_prev_on + channel, W
    selbnkc     triaczcc_cycle
    subwf       triaczcc_cycle, W
    selbnkc     triaczcc_delay + channel
    subwf       triaczcc_delay + channel, W
    btfsc       STATUS, C
    goto        next_end

    ;; triaczcc_prev_on[channel] += triaczcc_delay[channel]
    movf        triaczcc_delay + channel, W
    selbnkc     triaczcc_prev_on + channel
    addwf       triaczcc_prev_on + channel, F

    selbnkc     triaczcc_state + (channel / 8)
    bsf         triaczcc_state + (channel / 8), (channel % 8)
    set_macro   triaczcc_state + (channel / 8), (channel % 8)
next_end:
    endm

triaczcc_timer_next_ch  macro channel, set_macro, timer_value
    if channel >= TRIACZCC_NUM_CHANNELS
    error "triac_zcc channel out of bounds"
    endif

    local       next_end

    pagesel     next_end
    selbank     triaczcc_state + (channel / 8)
    btfss       triaczcc_state + (channel / 8), (channel % 8)
    goto        next_end

    ;; if (timer_value - zero_cross_time <= zero_cross_period / 2) goto next_end
    selbnkc     zero_cross_period
    movf        zero_cross_period, W
    movwf       acc1
    bcf         STATUS, C
    rrf         acc1, F
    selbnkc     zero_cross_time
    movf        zero_cross_time, W
    selbnkc     timer_value
    subwf       timer_value, W
    subwf       acc1, W
    btfsc       STATUS, C
    goto        next_end

    selbnkc     triaczcc_state + (channel / 8)
    bcf         triaczcc_state + (channel / 8), (channel % 8)
    set_macro   triaczcc_state + (channel / 8), (channel % 8)
next_end:
    endm

    ;; See scripts/zcc.py.
triaczcc_table_m  macro num_frac_bits, num_extra_bits
    local   i

    andlw   (1 << num_frac_bits) - 1
    table_jump  tbl
    retlw   0
i   set     1
    while   i < (1 << num_frac_bits)
    retlw   ((((1 << num_frac_bits) - 1) << (num_extra_bits + 1)) / i + 1) / 2
i   set     i + 1
    endw
    endm

triaczcc_table:
    ifndef TRIACZCC_NUM_FRAC_BITS
    error "TRIACZCC_NUM_FRAC_BITS is not defined"
    endif
    ifndef TRIACZCC_NUM_EXTRA_BITS
    error "TRIACZCC_NUM_EXTRA_BITS is not defined"
    endif
    triaczcc_table_m    TRIACZCC_NUM_FRAC_BITS, TRIACZCC_NUM_EXTRA_BITS

triaczcc_init   macro
    local   i

    selbank triaczcc_cycle
    clrf    triaczcc_cycle
i   set     0
    while   i < TRIACZCC_NUM_CHANNELS
    selbnkc triaczcc_delay + i
    clrf    triaczcc_delay + i
    selbnkc triaczcc_prev_on + i
    clrf    triaczcc_prev_on + i
i   set     i + 1
    endw
i   set     0
    while   i < TRIACZCC_NUM_CHANNELS / 8
    selbnkc triaczcc_state + (i / 8)
    clrf    triaczcc_state + (i / 8)
i   set     i + 1
    endw
    endm
    endif

    ifdef section_init
    triaczcc_init
    endif
