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
    endif

    ifdef section_code
zero_cross_int_next macro
    movf    zero_cross_time, W
    subwf   timer_value, W
    movwf   zero_cross_period

    movf    timer_value, W
    movwf   zero_cross_time
    endm
    endif

    ifdef section_init
    clrf    zero_cross_period
    clrf    zero_cross_time
    bsf     INTCON, INTEDG
    endif
