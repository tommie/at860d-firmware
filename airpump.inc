    ;; Airpump I/O module.
    ;;
    ;; RE1, active low

    ifdef module_airpump
#undefine module_airpump
    endif
#define module_airpump
    ifndef module_triac_pfc
    error "triac_pfc module is a dependency of airpump"
    endif

    ifdef section_udata
airpump_value res 1
airpump_ratio res 1
    endif ; section_udata

    ifdef section_code
airpump_output  macro   file, bit
    movinvbit           file, bit, PORTE, 1
    endm

airpump_timer_next macro  timer_value
    triacpfc_timer_next_ch  0, airpump_output, timer_value
    endm

airpump_int_next  macro
    triacpfc_int_next_ch    0, airpump_output
    endm

airpump_commitw macro
    ;; if (W) W += OFFSET
    ifndef  AIRPUMP_OFFSET
    error   "AIRPUMP_OFFSET is not defined"
    endif
    andlw   0xFF
    btfss   STATUS, Z
    addlw   AIRPUMP_OFFSET

    ;; if (!zero_cross_is_good()) W = 0
    zero_cross_is_not_good
    btfsc   STATUS, Z
    clrw

    selbank airpump_ratio
    movwf   airpump_ratio

    triacpfc_set    0, airpump_ratio
    endm

airpump_setw macro
    pagesel airpump_setw_func
    call    airpump_setw_func
    endm

airpump_setw_func:
    ;; if (W > MAX_RATIO) W = MAX_RATIO
    ifndef  AIRPUMP_MAX_RATIO
    error   "AIRPUMP_MAX_RATIO is not defined"
    endif
    addlw   -(AIRPUMP_MAX_RATIO)
    btfsc   STATUS, C
    clrw
    addlw   AIRPUMP_MAX_RATIO

    selbank airpump_value
    movwf   airpump_value

    airpump_commitw
    return

airpump_idle    macro
    local   mend

    ;; if (!airpump_value) goto mend
    pagesel mend
    selbank airpump_value
    movf    airpump_value, F
    btfsc   STATUS, Z
    goto    mend

    ;; if (airpump_ratio) goto mend
    selbank airpump_ratio
    movf    airpump_ratio, F
    btfss   STATUS, Z
    goto    mend

    ;; if (zero_cross_is_good()) airpump_setw
    zero_cross_is_not_good
    btfsc   STATUS, Z
    goto    mend

    selbank airpump_value
    movf    airpump_value, W
    airpump_commitw

mend:
    endm
    endif ; section_code

    ifdef section_init
    selbank PORTE
    bsf     PORTE, 1
    selbank TRISE
    bcf     TRISE, 1
    selbank airpump_value
    clrf    airpump_value
    selbank airpump_ratio
    clrf    airpump_ratio
    endif ; section_init

    ifdef section_idle
    airpump_idle
    endif ; section_idle
