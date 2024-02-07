    ;; Temperature Controller

    ifdef module_tempcontrol
#undefine module_tempcontrol
    endif
#define module_tempcontrol

    ifndef module_airpump
    error "airpump module is a dependency of tempcontrol"
    endif
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
TEMPC_P         equ 16
TEMPC_I         equ 0
TEMPC_D         equ 0
TEMPC_DT        equ 1           ; Time interval, in some unit.
TEMPC_FRAC      equ 0           ; The log2-divisor; determines how many full-range samples can be accumulated before saturation.
TEMPC_MAX_ERR   equ 20 << 1     ; 9.1 fixed point.
TEMPC_INTERVAL  equ 10          ; Time interval, in 1ms units.

    ;; tempc_state bits
TEMPC_ACTIVE    equ 0

tempc_value     res 2           ; 9.1 fixed point.
tempc_buf       res 2
tempc_next_t    res 2           ; In 1ms units.
tempc_state     res 1

    pid_lit_udata   0, 2, TEMPC_P, TEMPC_I, TEMPC_D, TEMPC_DT, TEMPC_FRAC, TEMPC_MAX_ERR
    endif ; section_udata

    ifdef section_code
tempc_idle  macro
    local   mend

    ;; if (!tempc_state.active) goto mend
    movlw   HIGH mend
    movwf   PCLATH
    selbank tempc_state
    btfss   tempc_state, TEMPC_ACTIVE
    goto    mend

    ifdef       TEMPC_DIRECT_CONTROL
    mov16ff     tempc_value, w16
    rr16fl      w16, 1, 0       ; To FP9.0
    selbnkc     w16
    movf        w16, W

    ;; heater_setw(airpump_value ? tempc_value/2 & 0xFF : 0)
    selbnkc     airpump_value
    movf        airpump_value, F
    btfsc       STATUS, Z
    clrw

    heater_setw
mend:
    exitm
    endif       ; TEMPC_DIRECT_CONTROL

    ;; if (tempc_next_t - timer_value > 0) goto mend
    movlw   HIGH mend
    movwf   PCLATH
    gts16ff tempc_next_t, timer_value
    btfsc   STATUS, C
    goto    mend

    ;; tempc_next_t += TEMPC_INTERVAL
    add16lf TEMPC_INTERVAL, tempc_next_t

    temp_get16      tempc_buf
    pid_lit_update  0, tempc_value, tempc_buf

    pid_get16   0, tempc_buf

    ;; tempc_buf = clamp(tempc_buf / 2^3, 0, 63)
    divp2s16lf  tempc_buf, 3
    clamps16f0l tempc_buf, (1 << (1 + TRIACZCC_NUM_FRAC_BITS)) - 1

    ;; heater_setw(airpump_value ? tempc_buf : 0)
    selbnkc     tempc_buf
    movf        tempc_buf, W

    selbnkc     airpump_value
    movf        airpump_value, F
    btfsc       STATUS, Z
    clrw

    heater_setw
mend:
    endm

tempc_set_active    macro   b
    selbank tempc_state
    if b
    bsf     tempc_state, TEMPC_ACTIVE
    else
    bcf     tempc_state, TEMPC_ACTIVE
    endif
    endm

    ifndef  TEMPCONTROL_MAX_TEMP
    error   "TEMPCONTROL_MAX_TEMP not defined"
    endif

    ;; Sets the set-temperature as *C in 9.1 fixed point.
tempc_set16 macro   file
    local   mend

    ;; if (file > TEMPCONTROL_MAX_TEMP) goto mend
    movlw   HIGH mend
    movwf   PCLATH
    ge16lf  file, (TEMPCONTROL_MAX_TEMP) + 1
    btfsc   STATUS, C
    goto    mend

    ;; if (file < TEMPCONTROL_MIN_TEMP) goto mend
    movlw   HIGH mend
    movwf   PCLATH
    ge16lf  file, TEMPCONTROL_MIN_TEMP
    btfss   STATUS, C
    goto    mend

    mov16ff file, tempc_value
mend:
    endm
    endif ; section_code

    ifdef section_init
    ifndef  TEMPCONTROL_MIN_TEMP
    error   "TEMPCONTROL_MIN_TEMP not defined"
    endif
    ifndef  TEMPC_DIRECT_CONTROL
    if      TEMPCONTROL_MIN_TEMP < TEMP_CONST_INIT
    error   "TEMPCONTROL_MIN_TEMP must be at least TEMP_CONST_INIT"
    endif
    endif   ; TEMPC_DIRECT_CONTROL

    mov16lf         TEMPCONTROL_MIN_TEMP, tempc_value
    mov16lf         1, tempc_next_t
    selbnkc         tempc_state
    clrf            tempc_state
    pid_lit_init    0
    endif ; section_init

    ifdef section_idle
    tempc_idle
    endif
