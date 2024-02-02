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
    endif

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

airpump_setw macro
    selbank             airpump_value
    movwf               airpump_value
    triacpfc_set        0, airpump_value
    endm
    endif

    ifdef section_init
    bsf     PORTE, 1
    selbank TRISE
    bcf     TRISE, 1
    selbnkc PORTE
    selbnkc airpump_value
    clrf    airpump_value
    endif
