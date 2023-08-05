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
    ifndef module_adc
    error "adc module is a dependency of cooldown"
    endif
    ifndef module_power
    error "power module is a dependency of cooldown"
    endif
    ifndef module_switch
    error "switch module is a dependency of cooldown"
    endif

    ifdef section_udata
cooldown_state      res 1
    endif

    ifdef section_code
cooldown_idle       macro   temp_value
    ifndef          COOLDOWN_MAX_TEMP
    error "COOLDOWN_MAX_TEMP is not defined"
    endif
    if              COOLDOWN_MAX_TEMP > 255
    error "COOLDOWN_MAX_TEMP larger than a byte"
    endif

    movlw           0x0F
    banksel         cooldown_state
    andwf           cooldown_state, F

    ;; cooldown_state_4 = temp_value > COOLDOWN_MAX_TEMP
    banksel         temp_value
    movf            temp_value, W
    sublw           COOLDOWN_MAX_TEMP
    banksel         cooldown_state
    btfss           STATUS, C
    bsf             cooldown_state, 4

    ;; power_set(cooldown_state_4)
    power_set       cooldown_state, 4

    ;; cooldown_state_5 = !swpower_get()
    swpower_get     cooldown_state, 5
    movlw           1 << 5
    xorwf           cooldown_state, F

    ;; cooldown_state_4 = cooldown_state_4 && cooldown_state_5
    btfss           cooldown_state, 5
    bcf             cooldown_state, 4

    ;; Commit new bits to lower nibble.
    swapf           cooldown_state, F
    endm

cooldown_skip_if_active  macro
    banksel         cooldown_state
    btfss           cooldown_state, 0
    endm

cooldown_skip_if_not_active  macro
    banksel         cooldown_state
    btfsc           cooldown_state, 0
    endm
    endif

    ifdef section_init
    banksel         cooldown_state
    clrf            cooldown_state
    endif

    ifdef section_idle
    cooldown_idle   adc_temp_value
    endif
