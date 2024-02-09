    ;; A fault monitor.
    ;;
    ;; Faults are terminal: once detected, it requires a reboot to clear.
    ;;
    ;; TEMP_SENSOR if temperature is over MAX_TEMP.
    ;; HEATER      if the heater has been on for more than HEATER_DELAY, but
    ;;             temperature is less than MIN_TEMP.

    ifdef module_fault
#undefine module_fault
    endif
#define module_fault

    ifndef module_temp
    error "temp module is a dependency of fault"
    endif

    ifdef section_udata
    ;; fault_state bits
FAULT_TEMP_SENSOR   equ 0
FAULT_HEATER        equ 1
FAULT_HEATER_EN     equ 7
FAULT_MASK          equ (1 << FAULT_TEMP_SENSOR) | (1 << FAULT_HEATER)

fault_state         res 1
fault_heater_t      res 1
    endif

    ifdef section_code
fault_idle_heater   macro
    local       heater_active
    local       heater_inactive
    local       mend

    ;; if (fault_state.heater) goto mend
    pagesel     mend
    btfsc       fault_state, FAULT_HEATER
    goto        mend

    ;; if (heater_is_inactive()) goto heater_inactive
    pagesel     heater_inactive
    heater_is_inactive
    selbank     fault_state
    btfsc       STATUS, Z
    goto        heater_inactive

    ;; if (temp_get16() >= MIN_TEMP) goto heater_inactive
    temp_get16  w16
    ge16lf      w16, FAULT_MIN_TEMP
    selbank     fault_state
    btfsc       STATUS, C
    goto        heater_inactive

    ;; if (fault_state.heater_en) goto heater_active
    pagesel     heater_active
    btfsc       fault_state, FAULT_HEATER_EN
    goto        heater_active

    ;; fault_heater_t = timer_value + HEATER_DELAY
    movlw       FAULT_HEATER_DELAY
    selbnkc     timer_value + 1
    addwf       timer_value + 1, W
    selbnkc     fault_heater_t
    movwf       fault_heater_t

    ;; fault_state.heater_en = 1
    selbnkc     fault_state
    bsf         fault_state, FAULT_HEATER_EN

heater_active:
    ;; if ((int) fault_heater_t - timer_value / 256 > 0) goto mend
    pagesel     mend
    selbank     fault_heater_t
    movf        fault_heater_t, W
    selbnkc     timer_value + 1
    subwf       timer_value + 1, W
    btfss       STATUS, C
    goto        mend

    bsf         fault_state, FAULT_HEATER

    pagesel     mend
    goto        mend

heater_inactive:
    ;; fault_state.heater_en = 0
    selbank     fault_state
    bcf         fault_state, FAULT_HEATER_EN
mend:
    endm

fault_idle_temp macro
    ;; if (temp_get16() >= MAX_TEMP) fault_state.temp_sensor = 1
    temp_get16  w16
    ge16lf      w16, FAULT_MAX_TEMP
    selbank     fault_state
    btfsc       STATUS, C
    bsf         fault_state, FAULT_TEMP_SENSOR
    endm

    ;; Sets Z to whether there is no fault.
    ;;
    ;; Clobbers W.
fault_is_good   macro
    ifdef       DISABLE_FAULT
    bcf         STATUS, Z
    else
    movlw       FAULT_MASK
    selbank     fault_state
    andwf       fault_state, W
    endif       ; DISABLE_FAULT
    endm

fault_skip_if_fault macro   bit
    selbank     fault_state
    btfss       fault_state, bit
    endm

fault_skip_if_not_fault macro   bit
    selbank     fault_state
    btfsc       fault_state, bit
    endm
    endif ; section_code

    ifdef section_init
    selbank fault_state
    clrf    fault_state

    ifdef   DISABLE_FAULT
    messg   "Fault detection is disabled by DISABLE_FAULT"
    endif
    endif ; section_init

    ifdef section_idle
    fault_idle_heater
    fault_idle_temp
    endif
