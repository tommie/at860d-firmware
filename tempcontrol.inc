    ;; Temperature Controller
    ;;
    ;; P - increase to decrease rise time and decrease error.
    ;; I - increase to remove steady-state error.
    ;; D - increase to remove oscillation/overshoot.
    ;;
    ;; See https://en.wikipedia.org/wiki/Proportional%E2%80%93integral%E2%80%93derivative_controller#Manual_tuning
    ;;
    ;; # Tuning From Scratch
    ;;
    ;; 1. Set P to one and I, D, FRAC and POSTFRAC to zero.
    ;;    Set MAX_ERR to 31 *C.
    ;; 2. Set airpump to max and set a temperature above 150 *C.
    ;; 3. Increase P until oscillation around some temperature.
    ;;    If the size of P increases above allowed bit size, increase FRAC
    ;;    and adjust P to compensate.
    ;; 4. Adjust MAX_ERR to be larger than half the oscillation peak-to-peak.
    ;;    Noting that a larger value may require P to shrink.
    ;; 5. To remove the steady-state error, set I to one.
    ;;    Temperature adjustment should feel very sluggish.
    ;; 6. Increase FRAC (and adjust P) until it feels rapid again.
    ;;    If FRAC is becoming close to the ADC value size (10 bits,)
    ;;    subtract some from FRAC and add to POSTFRAC.
    ;;    If the controller still cannot reach the set temperature,
    ;;    increase I, and keep adjusting FRAC and P to compensate.
    ;;    You may have to decrease P a bit, since the I term acts
    ;;    in the same direction.
    ;; 7. Increase D until oscillation goes away. If it is too coarse,
    ;;    increase FRAC and adjust P and I.

    ifdef module_tempcontrol
#undefine module_tempcontrol
    endif
#define module_tempcontrol

    ifndef module_airpump
    error "airpump module is a dependency of tempcontrol"
    endif
    ifndef module_fault
    error "fault module is a dependency of tempcontrol"
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
TEMPC_P         equ 384         ; Max 15 - log(MAX_ERR) + FRAC bits.
TEMPC_I         equ 1           ; This is pre-multiplied by dt.
TEMPC_D         equ 24          ; This is pre-divided by dt.
TEMPC_FRAC      equ 4           ; The log2-divisor; determines how many full-range samples can be accumulated before saturation.
TEMPC_POSTFRAC  equ 4           ; The log2-divisor on the PID output; determines the influence of the I-term.
TEMPC_MAX_ERR   equ 31 << 1     ; 5.1 fixed point.
TEMPC_MAX_ACC   equ (1 << (TRIACZCC_NUM_FRAC_BITS + TRIACZCC_NUM_EXTRA_BITS + TEMPC_POSTFRAC)) - 1
TEMPC_INTERVAL  equ 100         ; Time interval, in 1ms units.

    ;; tempc_state bits
TEMPC_ACTIVE    equ 0

tempc_value     res 2           ; 9.1 fixed point.
tempc_buf       res 2
tempc_next_t    res 2           ; In 1ms units.
tempc_state     res 1

    pid_lit_udata   0, 2, TEMPC_P, TEMPC_I, TEMPC_D, 1, TEMPC_FRAC, TEMPC_MAX_ERR, TEMPC_MAX_ACC
    endif ; section_udata

    ifdef section_code
tempc_idle  macro
    local   no_fault
    local   update_done
    local   mend

    ;; if (!tempc_state.active) goto mend
    movlw   HIGH mend
    movwf   PCLATH
    selbank tempc_state
    btfss   tempc_state, TEMPC_ACTIVE
    goto    mend

    ifdef   TEMPC_DIRECT_CONTROL
    mov16ff tempc_value, w16
    rr16fl  w16, 1, 0       ; To FP9.0
    selbnkc w16
    movf    w16, W

    ;; heater_setw(airpump_ratio ? tempc_value/2 & 0xFF : 0)
    selbnkc airpump_ratio
    movf    airpump_ratio, F
    btfsc   STATUS, Z
    clrw

    heater_setw
mend:
    exitm
    endif   ; TEMPC_DIRECT_CONTROL

    ;; if (tempc_next_t - timer_value > 0) goto mend
    movlw   HIGH mend
    movwf   PCLATH
    gts16ff tempc_next_t, timer_value
    btfsc   STATUS, C
    goto    mend

    ;; tempc_next_t += TEMPC_INTERVAL
    add16lf TEMPC_INTERVAL, tempc_next_t

    ;; if (!fault_is_good) tempc_buf = 0
    pagesel no_fault
    fault_is_good
    btfsc   STATUS, Z
    goto    no_fault

    clr16f  tempc_buf

    pagesel update_done
    goto    update_done

no_fault:
    temp_get16      tempc_buf
    pid_lit_update  0, tempc_value, tempc_buf

    pid_get16   0, tempc_buf
update_done:

    ;; tempc_buf /= 2^POSTFRAC
    divp2s16lf  tempc_buf, TEMPC_POSTFRAC

    ;; if (tempc_buf < 0) tempc_buf = 0
    mov16ff     tempc_buf, w16
    btfsc       w16 + 1, 7
    clrf        w16
    btfsc       w16 + 1, 7
    clrf        w16 + 1

    ;; heater_setw(airpump_ratio ? tempc_buf & 0xFF : 0)
    selbnkc     w16
    movf        w16, W
    selbnkc     airpump_ratio
    movf        airpump_ratio, F
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

    eq16ff  file, tempc_value
    pagesel mend
    btfsc   STATUS, Z
    goto    mend

    pid_lit_reset   0
    mov16ff         file, tempc_value
mend:
    endm
    endif ; section_code

    ifdef section_init
    ifndef  TEMPCONTROL_MIN_TEMP
    error   "TEMPCONTROL_MIN_TEMP not defined"
    endif
    ifdef   TEMPC_DIRECT_CONTROL
    messg   "Disabling tempcontrol: the heater is controlled open-loop!"
    else
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
