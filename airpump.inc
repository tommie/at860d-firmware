    ;; Airpump I/O module.
    ;;
    ;; RE1, active low

    ifdef module_airpump
#undefine module_airpump
    endif
#define module_airpump

    ifdef section_code
airpump_set macro   file, bit
    btfsc   file, bit
    bcf     PORTE, 1
    btfss   file, bit
    bsf     PORTE, 1
    endm
    endif

    ifdef section_init
    bsf     PORTE, 1
    banksel TRISE
    bcf     TRISE, 1
    banksel PORTE
    endif
