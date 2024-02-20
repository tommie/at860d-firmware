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
    pagesel         not_enabled
    cooldown_skip_if_not_active
    goto            not_enabled
    endif

    pagesel         handle_in_cradle
    swstandby_get   STATUS, C
    btfsc           STATUS, C
    goto            handle_in_cradle

    ;; Not in cradle.
not_enabled:
    selbank         standby_time
    clrf            standby_time
    selbank         standby_state
    bcf             standby_state, STANDBY_ACTIVE
    pagesel         idle_end
    goto            idle_end

handle_in_cradle:
    pagesel         time_set
    selbank         standby_time
    movf            standby_time, F
    btfss           STATUS, Z
    goto            time_set

    ;; standby_time = timer_value%256 + (timer_value%256 == 0 ? 1 : 0)
    selbank         timer_value
    movf            timer_value, W
    btfsc           STATUS, Z
    addlw           1
    selbank         standby_time
    movwf           standby_time

time_set:
    ;; if ((int) timer_value%256 - (standby_time + delay) > 0) standby_state |= 1 << STANDBY_ACTIVE
    ifndef          STANDBY_AIR
    error "STANDBY_AIR is not defined"
    endif
    ifndef          STANDBY_DELAY
    error "STANDBY_DELAY is not defined"
    endif
    ifndef          STANDBY_MAX_TEMP
    error "STANDBY_MAX_TEMP is not defined"
    endif
    if              STANDBY_DELAY >= 1 << 7
    error "STANDBY_DELAY larger than a signed byte"
    endif
    selbank         standby_time
    movf            standby_time, W
    addlw           STANDBY_DELAY
    selbank         timer_value
    subwf           timer_value, W
    andlw           0x80
    selbank         standby_state
    btfsc           STATUS, Z
    bsf             standby_state, STANDBY_ACTIVE
idle_end:

    ;; if (!standby_active) goto not_active
    pagesel         not_active
    standby_skip_if_active
    goto            not_active

    selbank         standby_state
    bsf             standby_state, STANDBY_WAS_ACTIVE

    ;; airpump_setw(temp_get16() > STANDBY_MAX_TEMP ? STANDBY_AIR : 0)
    temp_get16      w16
    gt16fl          w16, STANDBY_MAX_TEMP
    movlw           STANDBY_AIR
    btfss           STATUS, C
    clrw
    airpump_setw

    tempc_set_active    0

    ;; A failsafe in case tempcontrol is broken.
    clrw
    heater_setw

    pagesel         mend
    goto            mend

not_active:
    ;; if (!STANDBY_WAS_ACTIVE) goto mend
    pagesel         mend
    selbank         standby_state
    btfss           standby_state, STANDBY_WAS_ACTIVE
    goto            mend

    bcf                 standby_state, STANDBY_WAS_ACTIVE
    tempc_set_active    1
    uc_load             0

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

    ;; Skips the next instruction if the temperature is low enough to be safe.
standby_skip_if_sleep   macro
    temp_get16  w16
    gt16fl      w16, STANDBY_MAX_TEMP
    bcf         STATUS, DC
    btfss       STATUS, C
    bsf         STATUS, DC
    standby_skip_if_active
    bcf         STATUS, DC
    btfss       STATUS, DC
    endm
    endif

    ifdef section_init
    selbank     standby_time
    clrf        standby_time
    selbank     standby_state
    clrf        standby_state
    endif

    ifdef section_idle
    standby_idle    timer_value + 1
    endif
