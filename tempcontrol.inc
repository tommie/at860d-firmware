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
    ifndef module_timer0
    error "timer0 module is a dependency of tempcontrol"
    endif
    ifndef module_temp
    error "temp module is a dependency of tempcontrol"
    endif

    ifdef section_udata
TEMPC_P         equ 64
TEMPC_I         equ 8
TEMPC_D         equ 16
TEMPC_DT        equ 1           ; Time interval, in some unit.
TEMPC_FRAC      equ 0           ; The log2-divisor, interpreted as the number of fractional bits in parameters.

tempc_value     res 2           ; 9.1 fixed point.
tempc_buf       res 2
tempc_next_t    res 1           ; In units of 256*timer_value.

    pid_lit_udata   0, 2, TEMPC_P, TEMPC_I, TEMPC_D, TEMPC_DT, TEMPC_FRAC
    endif

    ifdef section_code
tempc_idle  macro
    local   mend

    ;; TODO: change to inequality. Not strictly needed, since it's a slow timer.
    ;; if (timer_value/256 != tempc_next_t) goto mend
    selbank timer_value + 1
    movf    timer_value + 1, W
    selbnkc tempc_next_t
    subwf   tempc_next_t, W
    movlw   HIGH mend
    movwf   PCLATH
    btfss   STATUS, Z
    goto    mend

    ;; tempc_next_t++
    selbnkc tempc_next_t
    incf    tempc_next_t, F

    ifdef TEMPC_DIRECT_CONTROL
    selbnkc     tempc_value
    movf        tempc_value, W
    andlw       (1 << (2 + TRIACZCC_NUM_FRAC_BITS)) - 1
    heater_setw
mend:
    exitm
    endif

    temp_get16      tempc_buf
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
mend:
    endm

    ;; Sets the set-temperature as *C in 9.1 fixed point.
tempc_set16 macro   file
    mov16ff file, tempc_value
    endm
    endif

    ifdef section_init
    clr16f          tempc_value
    selbnkc         tempc_next_t
    clrf            tempc_next_t
    incf            tempc_next_t, F
    pid_lit_init    0
    endif

    ifdef section_idle
    tempc_idle
    endif
