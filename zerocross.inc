    ;; Mains zero-cross detection module.
    ;;
    ;; RB0, active low
    ;;
    ;; zero_cross_time goes up to ~128 ms. zero_cross_period only works down to 7.8 Hz
    ;;
    ;; Note this has a cyclic dependency on timer0. It should be a reverse dependency.

    ifdef module_zerocross
#undefine module_zerocross
    endif
#define module_zerocross

    ifdef section_udata
zero_cross_period   res 1
zero_cross_time     res 1
    endif

    ifdef section_code
zero_cross_int_next macro
    local   set_period
    local   set_end

    pagesel set_end

    ;; W = timer1_value - zero_cross_time
    selbank zero_cross_time
    movf    zero_cross_time, W
    selbnkc timer1_value
    subwf   timer1_value, W

    ;; if (W > 100) goto set_end
    addlw   -100
    btfsc   STATUS, C
    goto    set_end
    addlw   100

    movwf   acc1
    pagesel set_period
    movf    acc1, W

    ;; if (zero_cross_period == 0) zero_cross_period += W
    ;; else zero_cross_period += (W - zero_cross_period) / 8
    selbnkc zero_cross_period
    movf    zero_cross_period, F
    btfsc   STATUS, Z
    goto    set_period

    ;; W = (-(zero_cross_period - W) + 4) / 8
    subwf   zero_cross_period, W
    xorlw   0xFF
    addlw   1 + 4
    selbnkc acc1
    movwf   acc1
    bcf     STATUS, C
    btfsc   acc1, 7
    bsf     STATUS, C
    rrf     acc1, F
    bcf     STATUS, C
    btfsc   acc1, 7
    bsf     STATUS, C
    rrf     acc1, F
    bcf     STATUS, C
    btfsc   acc1, 7
    bsf     STATUS, C
    rrf     acc1, F
    movf    acc1, W

set_period:
    selbank zero_cross_period
    addwf   zero_cross_period, F
set_end:

    selbank timer1_value
    movf    timer1_value, W
    selbnkc zero_cross_time
    movwf   zero_cross_time
    endm

    ;; Sets Z to whether zero_cross_period is invalid.
zero_cross_is_not_good  macro
    selbank zero_cross_period
    movf    zero_cross_period, F
    endm
    endif

    ifdef section_init
    selbank zero_cross_period
    clrf    zero_cross_period
    selbnkc zero_cross_time
    clrf    zero_cross_time
    bsf     INTCON, INTEDG
    endif
