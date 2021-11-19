    ;; Standby timer module.
    ;;
    ;; If the handle is in the cradle for long, output is disabled
	;; until it is lifted again.
    ;;
    ;; This module has an implicit dependency on timer0 (timer_value),
	;; but an explicit one would cause a cycle.

    ifdef module_standby
#undefine module_standby
    endif
#define module_standby
    ifndef module_switch
    error "switch module is a dependency of standby"
    endif

    ifdef section_udata
standby_time    res 1
standby_state   res 1
    endif

    ifdef section_code
standby_idle    macro   timer_value
    local           handle_in_cradle
    local           time_set
    local           idle_end

    movlw           HIGH handle_in_cradle
    movwf           PCLATH
    swstandby_get   STATUS, C
    btfsc           STATUS, C
    goto            handle_in_cradle

    ;; Not in cradle.
    clrf            standby_time
    bcf             standby_state, 0
    movlw           HIGH idle_end
    movwf           PCLATH
    goto            idle_end

handle_in_cradle:
    movlw           HIGH time_set
    movwf           PCLATH
    movf            standby_time, F
    btfss           STATUS, Z
    goto            time_set

    ;; standby_time = timer_value + (timer_value == 0 ? 1 : 0)
    movf            timer_value, W
    btfsc           STATUS, Z
    addlw           1
    movwf           standby_time

time_set:
    ;; if (timer_value - standby_time > delay) standby_state = 1
    ifndef      STANDBY_DELAY
    error "STANDBY_DELAY is not defined"
    endif
    if          STANDBY_DELAY > 255
    error "STANDBY_DELAY larger than a byte"
    endif
    movf        standby_time, W
    subwf       timer_value, W
    sublw       STANDBY_DELAY
    btfss       STATUS, C
    bsf         standby_state, 0

idle_end:
    endm

standby_skip_if_active  macro
    btfss       standby_state, 0
    endm

standby_skip_if_not_active  macro
    btfsc       standby_state, 0
    endm
    endif

    ifdef section_init
    clrf            standby_time
    clrf            standby_state
    endif

    ifdef section_idle
    standby_idle    timer_value + 1
    endif
