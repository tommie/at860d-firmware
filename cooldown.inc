    ;; Cooldown module.
    ;;
    ;; If the temperature is higher than some value, the power bypass is enabled.
    ;;
    ;; States:
    ;; 0 - The device is cool enough that no cooldown phase is needed.
    ;; 1 - The device is (or has been) too hot. It will continue running until cool enough.

    ifdef module_cooldown
#undefine module_cooldown
    endif
#define module_cooldown
    ifndef module_airpump
    error "airpump module is a dependency of standby"
    endif
    ifndef module_heater
    error "heater module is a dependency of cooldown"
    endif
    ifndef module_power
    error "power module is a dependency of cooldown"
    endif
    ifndef module_switch
    error "switch module is a dependency of cooldown"
    endif
    ifndef module_temp
    error "temp module is a dependency of cooldown"
    endif
    ifndef module_tempcontrol
    error "tempcontrol module is a dependency of cooldown"
    endif

    ifdef section_udata
cooldown_state      res 1
    endif

    ifdef section_code
cooldown_idle       macro
    local           active_done

    ifndef          COOLDOWN_AIR
    error "COOLDOWN_AIR is not defined"
    endif
    ifndef          COOLDOWN_MAX_TEMP
    error "COOLDOWN_MAX_TEMP is not defined"
    endif
    if              COOLDOWN_MAX_TEMP >= (1 << 16)
    error "COOLDOWN_MAX_TEMP larger than two bytes"
    endif

    movlw           0x0F
    selbank         cooldown_state
    andwf           cooldown_state, F

    ;; cooldown_state_4 = temp_get16() > COOLDOWN_MAX_TEMP
    temp_get16      w16
    gt16fl          w16, COOLDOWN_MAX_TEMP
    selbank         cooldown_state
    btfsc           STATUS, C
    bsf             cooldown_state, 4

    ;; power_set(cooldown_state_4)
    power_set       cooldown_state, 4

    ;; cooldown_state_5 = !swpower_get()
    movlw           1 << 5
    swpower_skip_if_clear
    clrw
    selbank         cooldown_state
    bcf             cooldown_state, 5
    iorwf           cooldown_state, F

    ;; cooldown_state_4 = (cooldown_state_4 && cooldown_state_5) || cooldown_state_0
    btfss           cooldown_state, 5
    bcf             cooldown_state, 4
    btfsc           cooldown_state, 0
    bsf             cooldown_state, 4

    ;; Commit new bits to lower nibble.
    swapf           cooldown_state, F

    ;; if (!cooldown_active) goto active_done
    pagesel         active_done
    cooldown_skip_if_active
    goto            active_done

    movlw           COOLDOWN_AIR
    airpump_setw

    tempc_set_active    0

    clrw
    heater_setw
active_done:
    endm

cooldown_skip_if_active  macro
    selbank         cooldown_state
    btfss           cooldown_state, 0
    endm

cooldown_skip_if_not_active  macro
    selbank         cooldown_state
    btfsc           cooldown_state, 0
    endm
    endif

    ifdef section_init
    selbank         cooldown_state
    clrf            cooldown_state
    endif

    ifdef section_idle
    cooldown_idle
    endif
