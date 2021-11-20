    ;; Mains zero-cross detection module.
    ;;
    ;; RB0, active low
    ;;
    ;; timer_value goes up to ~128 ms. zero_cross_period only works down to 7.8 Hz
    ;;
    ;; Note this has a cyclic dependency on timer0. It should be a reverse dependency.

    ifdef module_zerocross
#undefine module_zerocross
    endif
#define module_zerocross

    ifdef section_udata
zero_cross_period   res 1
zero_cross_time     res 1
zero_cross_acc      res 1
    endif

    ifdef section_code
zero_cross_int_next macro
    local   set_period
    local   set_end

    movlw   HIGH set_end
    movwf   PCLATH

    ;; W = timer_value - zero_cross_time
    movf    zero_cross_time, W
    subwf   timer_value, W

    ;; if (W > 50) goto set_end
    addlw   -50
    btfsc   STATUS, C
    goto    set_end
    addlw   50

    movwf   acc1
    movlw   HIGH set_period
    movwf   PCLATH
    movf    acc1, W

    ;; if (zero_cross_period == 0) zero_cross_period += W
    ;; else zero_cross_period += (W - zero_cross_period) / 8
    movf    zero_cross_period, F
    btfsc   STATUS, Z
    goto    set_period

    ;; W = (-(zero_cross_period - W) + 4) / 8
    subwf   zero_cross_period, W
    xorlw   0xFF
    addlw   1 + 4
    movwf   zero_cross_acc
    bcf     STATUS, C
    btfsc   zero_cross_acc, 7
    bsf     STATUS, C
    rrf     zero_cross_acc, F
    bcf     STATUS, C
    btfsc   zero_cross_acc, 7
    bsf     STATUS, C
    rrf     zero_cross_acc, F
    bcf     STATUS, C
    btfsc   zero_cross_acc, 7
    bsf     STATUS, C
    rrf     zero_cross_acc, F
    movf    zero_cross_acc, W

set_period:
    addwf   zero_cross_period, F
set_end:

    movf    timer_value, W
    movwf   zero_cross_time
    endm
    endif

    ifdef section_init
    clrf    zero_cross_period
    clrf    zero_cross_time
    bsf     INTCON, INTEDG
    endif
