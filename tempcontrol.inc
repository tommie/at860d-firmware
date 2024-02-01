    ;; Temperature Controller

    ifdef module_tempcontrol
#undefine module_tempcontrol
    endif
#define module_tempcontrol

    ifndef module_heater
    error "heater module is a dependency of tempcontrol"
    endif
    ifndef module_pid
    error "pid module is a dependency of tempcontrol"
    endif
    ifndef module_temp
    error "temp module is a dependency of tempcontrol"
    endif

    ifdef section_udata
TEMPC_P         equ 4
TEMPC_I         equ 1
TEMPC_D         equ 1
TEMPC_DT        equ 1           ; Time interval, in some unit.
TEMPC_FRAC      equ 1           ; The log2-divisor, interpreted as the number of fractional bits in parameters.

tempc_value     res 2           ; 9.1 fixed point.
tempc_buf       res 2

    pid_lit_udata   0, 2, TEMPC_P, TEMPC_I, TEMPC_D, TEMPC_DT, TEMPC_FRAC
    endif

    ifdef section_code
tempc_idle  macro
    ifdef TEMPC_DIRECT_CONTROL
    selbank     tempc_value
    movf        tempc_value, W
    andlw       (1 << (2 + TRIACZCC_NUM_FRAC_BITS)) - 1
    heater_setw
    exitm
    endif

    temp_get16  tempc_buf
    ;mov16lf     100 << 1, tempc_buf

    pid_lit_update  0, tempc_value, tempc_buf

    pid_get16   0, tempc_buf
    divp2s16lf  tempc_buf, 15 - (1 + TRIACZCC_NUM_FRAC_BITS) ; Truncated, not rounded, to avoid rounding up to one more bit.

    ;; heater_setw(tempc_buf < 0 ? 0 : tempc_buf)
    selbnkc tempc_buf
    movf    tempc_buf, W
    selbnkc tempc_buf + 1
    btfsc   tempc_buf + 1, 7
    clrw
    heater_setw
    endm

    ;; Sets the set-temperature as *C in 9.1 fixed point.
tempc_set16 macro   file
    mov16ff file, tempc_value
    endm
    endif

    ifdef section_init
    clr16f          tempc_value
    pid_lit_init    0
    endif

    ifdef section_idle
    tempc_idle
    endif
