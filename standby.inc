    ;; Standby timer module.
    ;;
    ;; If the handle is in the cradle for long, output is disabled
    ;; until it is lifted again.

    ifdef module_standby
#undefine module_standby
    endif
#define module_standby
    ifndef module_airpump
    error "airpump module is a dependency of standby"
    endif
    ifndef DISABLE_COOLDOWN
    ifndef module_cooldown
    error "cooldown module is a dependency of standby"
    endif
    endif
    ifndef module_switch
    error "switch module is a dependency of standby"
    endif
    ifndef module_heater
    error "heater module is a dependency of standby"
    endif
    ifndef module_temp
    error "temp module is a dependency of standby"
    endif
    ifndef module_tempcontrol
    error "tempcontrol module is a dependency of standby"
    endif
    ifndef module_timer0
    error "timer0 module is a dependency of standby"
    endif
    ifndef module_userconfig
    error "userconfig module is a dependency of standby"
    endif

    ifdef section_udata
    ;; standby_state bits
STANDBY_ACTIVE      equ 0
STANDBY_WAS_ACTIVE  equ 1

standby_time    res 1           ; In units of 128 ms.
standby_state   res 1
    endif

    ifdef section_code
standby_idle    macro   timer_value
    local           not_enabled
    local           handle_in_cradle
    local           time_set
    local           idle_end
    local           not_active
    local           mend

    ifndef DISABLE_COOLDOWN
    ;; if (cooldown_active) goto not_enabled
    movlw           HIGH not_enabled
    movwf           PCLATH
    cooldown_skip_if_not_active
    goto            not_enabled
    endif

    movlw           HIGH handle_in_cradle
    movwf           PCLATH
    swstandby_get   STATUS, C
    btfsc           STATUS, C
    goto            handle_in_cradle

    ;; Not in cradle.
not_enabled:
    selbank         standby_time
    clrf            standby_time
    selbnkc         standby_state
    bcf             standby_state, STANDBY_ACTIVE
    movlw           HIGH idle_end
    movwf           PCLATH
    goto            idle_end

handle_in_cradle:
    movlw           HIGH time_set
    movwf           PCLATH
    selbank         standby_time
    movf            standby_time, F
    btfss           STATUS, Z
    goto            time_set

    ;; standby_time = timer_value + (timer_value == 0 ? 1 : 0)
    selbnkc         timer_value
    movf            timer_value, W
    btfsc           STATUS, Z
    addlw           1
    selbnkc         standby_time
    movwf           standby_time

time_set:
    ;; if (timer_value - standby_time > delay) standby_state |= 1 << STANDBY_ACTIVE
    ifndef          STANDBY_AIR
    error "STANDBY_AIR is not defined"
    endif
    ifndef          STANDBY_DELAY
    error "STANDBY_DELAY is not defined"
    endif
    ifndef          STANDBY_MAX_TEMP
    error "STANDBY_MAX_TEMP is not defined"
    endif
    if              STANDBY_DELAY > 255
    error "STANDBY_DELAY larger than a byte"
    endif
    selbank         standby_time
    movf            standby_time, W
    selbnkc         timer_value
    subwf           timer_value, W
    sublw           STANDBY_DELAY
    selbnkc         standby_state
    btfss           STATUS, C
    bsf             standby_state, STANDBY_ACTIVE
idle_end:

    ;; if (!standby_active) goto not_active
    movlw           HIGH not_active
    movwf           PCLATH
    standby_skip_if_active
    goto            not_active

    selbank         standby_state
    bsf             standby_state, STANDBY_WAS_ACTIVE

    ;; airpump_setw(temp_get16() > STANDBY_MAX_TEMP ? 64 : 0)
    temp_get16      w16
    gt16fl          w16, STANDBY_MAX_TEMP
    movlw           STANDBY_AIR
    btfsc           STATUS, C
    clrw
    airpump_setw

    clr16f      w16
    tempc_set16 w16

    ;; A failsafe in case tempcontrol is broken.
    clrw
    heater_setw

    movlw           HIGH mend
    movwf           PCLATH
    goto            mend

not_active:
    ;; if (!STANDBY_WAS_ACTIVE) goto mend
    movlw           HIGH mend
    movwf           PCLATH
    selbank         standby_state
    btfss           standby_state, STANDBY_WAS_ACTIVE
    goto            mend

    bcf             standby_state, STANDBY_WAS_ACTIVE
    uc_load         0

mend:
    endm

standby_skip_if_active  macro
    selbank     standby_state
    btfss       standby_state, 0
    endm

standby_skip_if_not_active  macro
    selbank     standby_state
    btfsc       standby_state, 0
    endm
    endif

    ifdef section_init
    selbank     standby_time
    clrf        standby_time
    selbnkc     standby_state
    clrf        standby_state
    endif

    ifdef section_idle
    standby_idle    timer_value + 1
    endif
