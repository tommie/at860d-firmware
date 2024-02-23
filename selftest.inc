    ;; Startup self-test module.
    ;;
    ;; Dependency tree of the tests:
    ;; * Check that the power switch is on.
    ;; * Check that a button scan has happened (again) on row 0.
    ;;   * Check that a button scan has happened on row 2.
    ;;     * Check that no buttons are pressed.
    ;; * Check that the switches are not flickering.
    ;; * Check that the zero-cross period is reasonable and stable.
    ;; * Check that the temperature ADC is stable.
    ;;   * Check that the temperature is room temperature, or falling slowly.
    ;;     * (slow) Check that heating increases temperature.
    ;;       * (slow) Check that high airflow decreases temperature quickly.

    ifdef module_selftest
#undefine module_selftest
    endif
#define module_selftest
    ifndef module_adc
    error "adc module is a dependency of selftest"
    endif
    ifndef module_airpump
    error "airpump module is a dependency of selftest"
    endif
    ifndef module_buttonscan
    error "buttonscan module is a dependency of selftest"
    endif
    ifndef module_heater
    error "heater module is a dependency of selftest"
    endif
    ifndef module_switch
    error "switch module is a dependency of selftest"
    endif
    ifndef module_zerocross
    error "zerocross module is a dependency of selftest"
    endif

    ifdef section_udata
selftest_start_time res 2
selftest_state      res 2
selftest_switches   res 1
selftest_temp       res 1

SELFTEST_S_POWER    equ 0
SELFTEST_S_BTNSCAN2 equ 1
SELFTEST_S_BTNSCAN0 equ 2
SELFTEST_S_BUTTONS  equ 3
SELFTEST_S_SWITCHES equ 4
SELFTEST_S_ZCPERIOD equ 5
SELFTEST_S_TEMPADC  equ 6
SELFTEST_S_TEMP     equ 7
SELFTEST_S_HEATER   equ 8
SELFTEST_S_AIRPUMP  equ 9
SELFTEST_S_ETIMEOUT equ 14
SELFTEST_S_PASSED   equ 15
SELFTEST_FAST_TESTS equ 0x00FF
SELFTEST_SLOW_TESTS equ 0x0300
SELFTEST_ALL_TESTS  equ SELFTEST_SLOW_TESTS | SELFTEST_FAST_TESTS
SELFTEST_FAILURES   equ (1 << SELFTEST_S_ETIMEOUT)

SELFTEST_SWITCHES_DURATION  equ 50000 / 500     ; In 500 µs units.
SELFTEST_ZCPERIOD_DURATION  equ 3*2000 / 50     ; In 500 µs units; 3 cycles at 50 Hz.
SELFTEST_ZCPERIOD_MIN       equ 2000 / 60 - 4   ; In 500 µs units.
SELFTEST_ZCPERIOD_MAX       equ 2000 / 50 + 4   ; In 500 µs units.
SELFTEST_TEMPADC_DURATION   equ 50000 / 500     ; In 500 µs units.
SELFTEST_TEMPADC_DEV        equ 2
SELFTEST_TEMP_AMB_MAX       equ 10
SELFTEST_TEMP_AIRFLOW       equ 64
SELFTEST_HEATER_AIRFLOW     equ AIRPUMP_MAX_RATIO
SELFTEST_HEATER_TEMP_INC    equ 5
SELFTEST_AIRPUMP_AIRFLOW    equ AIRPUMP_MAX_RATIO
    endif

    ifdef section_code
selftest_t_power    macro   timer_value
    local   t_end

    pagesel t_end
    selbank selftest_state + SELFTEST_S_POWER / 8
    btfsc   selftest_state + SELFTEST_S_POWER / 8, SELFTEST_S_POWER % 8
    goto    t_end

    clrw
    swpower_skip_if_clear
    movlw   1 << (SELFTEST_S_POWER % 8)
    selbank selftest_state + SELFTEST_S_POWER / 8
    iorwf   selftest_state + SELFTEST_S_POWER / 8, F

    mov16ff timer_value, selftest_start_time
t_end:
    endm

selftest_t_btnscan2 macro
    local   t_end

    pagesel t_end
    selbank selftest_state + SELFTEST_S_BTNSCAN2 / 8
    btfsc   selftest_state + SELFTEST_S_BTNSCAN2 / 8, SELFTEST_S_BTNSCAN2 % 8
    goto    t_end

    movlw   2
    selbank buttonscan_i
    subwf   buttonscan_i, W
    selbank selftest_state + SELFTEST_S_BTNSCAN2 / 8
    btfsc   STATUS, Z
    bsf     selftest_state + SELFTEST_S_BTNSCAN2 / 8, SELFTEST_S_BTNSCAN2 % 8
t_end:
    endm

    ;; Requires S_BTNSCAN2.
selftest_t_btnscan0 macro
    local   t_end

    pagesel t_end
    selbank selftest_state + SELFTEST_S_BTNSCAN2 / 8
    btfss   selftest_state + SELFTEST_S_BTNSCAN2 / 8, SELFTEST_S_BTNSCAN2 % 8
    goto    t_end

    selbank selftest_state + SELFTEST_S_BTNSCAN0 / 8
    btfsc   selftest_state + SELFTEST_S_BTNSCAN0 / 8, SELFTEST_S_BTNSCAN0 % 8
    goto    t_end

    selbank buttonscan_i
    movf    buttonscan_i, F
    selbank selftest_state + SELFTEST_S_BTNSCAN0 / 8
    btfsc   STATUS, Z
    bsf     selftest_state + SELFTEST_S_BTNSCAN0 / 8, SELFTEST_S_BTNSCAN0 % 8
t_end:
    endm


    ;; Requires S_BTNSCAN0.
selftest_t_buttons macro
    local   t_end

    pagesel t_end
    selbank selftest_state + SELFTEST_S_BTNSCAN0 / 8
    btfss   selftest_state + SELFTEST_S_BTNSCAN0 / 8, SELFTEST_S_BTNSCAN0 % 8
    goto    t_end

    selbank selftest_state + SELFTEST_S_BUTTONS / 8
    btfsc   selftest_state + SELFTEST_S_BUTTONS / 8, SELFTEST_S_BUTTONS % 8
    goto    t_end

    selbank buttonscan_current
    movf    buttonscan_current, F
    btfss   STATUS, Z
    goto    t_end

    selbank buttonscan_current + 1
    movf    buttonscan_current + 1, F
    selbank selftest_state + SELFTEST_S_BUTTONS / 8
    btfsc   STATUS, Z
    bsf     selftest_state + SELFTEST_S_BUTTONS / 8, SELFTEST_S_BUTTONS % 8
t_end:
    endm

selftest_get_switches   macro   file, first_bit
    clrw
    swcal_skip_if_clear
    iorlw   1 << (first_bit + 0)
    swset_skip_if_clear
    iorlw   1 << (first_bit + 1)
    swstandby_skip_if_clear
    iorlw   1 << (first_bit + 2)
    swpower_skip_if_clear
    iorlw   1 << (first_bit + 3)
    selbank file
    movwf   file
    endm

selftest_t_switches macro   timer_value
    local           t_ok
    local           t_end

    pagesel         t_end
    selbank         selftest_state + SELFTEST_S_SWITCHES / 8
    btfsc           selftest_state + SELFTEST_S_SWITCHES / 8, SELFTEST_S_SWITCHES % 8
    goto            t_end

    selftest_get_switches   acc1, 4

    ;; Record if any switch has ever flickered.
    ;; selftest_switches = ((((selftest_switches ^ acc1) >> 4) | selftest_switches) & 0x0F) | acc1
    movf            acc1, W
    selbank         selftest_switches
    xorwf           selftest_switches, F
    swapf           selftest_switches, W
    iorwf           selftest_switches, W
    andlw           0x0F
    iorwf           acc1, W
    movwf           selftest_switches

    ;; if ((int) timer_value % 256 - (selftest_start_time % 256 + SELFTEST_SWITCHES_DURATION) < 0) goto t_end
    if              SELFTEST_SWITCHES_DURATION > 127
    error           "Assertion failed: SELFTEST_SWITCHES_DURATION < 2^7"
    endif
    selbank         selftest_start_time
    movf            selftest_start_time, W
    addlw           SELFTEST_SWITCHES_DURATION
    selbank         timer_value
    subwf           timer_value, W
    andlw           0x80
    btfsc           STATUS, Z
    goto            t_end

    ;; if ((selftest_switches & 0x0F) == 0) goto t_ok
    movlw           0x0F
    selbank         selftest_switches
    andwf           selftest_switches, W
    pagesel         t_ok
    btfsc           STATUS, Z
    goto            t_ok

    ;; selftest_switches = acc1
    movf            acc1, W
    movwf           selftest_switches

    ;; selftest_start_time = timer_value
    mov16ff         timer_value, selftest_start_time

    pagesel         t_end
    goto            t_end

t_ok:
    selbank         selftest_state + SELFTEST_S_SWITCHES / 8
    bsf             selftest_state + SELFTEST_S_SWITCHES / 8, SELFTEST_S_SWITCHES % 8

t_end:
    endm

    ;; This resets selftest_start_time until the period is reasonable.
selftest_t_zcperiod macro   timer_value
    local   t_not_ok
    local   t_end

    pagesel t_end
    selbank selftest_state + SELFTEST_S_ZCPERIOD / 8
    btfsc   selftest_state + SELFTEST_S_ZCPERIOD / 8, SELFTEST_S_ZCPERIOD % 8
    goto    t_end

    ;; if (zero_cross_period <= SELFTEST_ZCPERIOD_MIN) selftest_start_time = timer_value
    pagesel t_not_ok
    selbank zero_cross_period
    movf    zero_cross_period, W
    sublw   SELFTEST_ZCPERIOD_MIN
    btfsc   STATUS, C
    goto    t_not_ok

    ;; if (zero_cross_period > SELFTEST_ZCPERIOD_MAX) selftest_start_time = timer_value
    selbank zero_cross_period
    movf    zero_cross_period, W
    sublw   SELFTEST_ZCPERIOD_MAX
    btfss   STATUS, C
    goto    t_not_ok

    ;; if ((int) timer_value % 256 - (selftest_start_time % 256 + SELFTEST_ZCPERIOD_DURATION) < 0) goto t_end
    if      SELFTEST_ZCPERIOD_DURATION > 127
    error   "Assertion failed: SELFTEST_ZCPERIOD_DURATION < 2^7"
    endif
    pagesel t_end
    selbank selftest_start_time
    movf    selftest_start_time, W
    addlw   SELFTEST_ZCPERIOD_DURATION
    selbank timer_value
    subwf   timer_value, W
    andlw   0x80
    btfsc   STATUS, Z
    goto    t_end

    selbank selftest_state + SELFTEST_S_ZCPERIOD / 8
    bsf     selftest_state + SELFTEST_S_ZCPERIOD / 8, SELFTEST_S_ZCPERIOD % 8

    goto    t_end

t_not_ok:
    ;; selftest_start_time = timer_value
    mov16ff         timer_value, selftest_start_time

t_end:
    endm

selftest_t_tempadc  macro   timer_value
    local   t_not_ok
    local   t_end

    pagesel t_end
    selbank selftest_state + SELFTEST_S_TEMPADC / 8
    btfsc   selftest_state + SELFTEST_S_TEMPADC / 8, SELFTEST_S_TEMPADC % 8
    goto    t_end

    ;; selftest_temp += (adc_temp_value - selftest_temp +- 2) / 4
    selbank selftest_temp
    movf    selftest_temp, W
    selbank adc_temp_value
    subwf   adc_temp_value, W
    movwf   acc1
    movlw   2
    btfss   acc1, 7
    addwf   acc1, F
    btfsc   acc1, 7
    subwf   acc1, F
    bcf     STATUS, C
    btfsc   acc1, 7
    bsf     STATUS, C
    rrf     acc1, F
    bcf     STATUS, C
    btfsc   acc1, 7
    bsf     STATUS, C
    rrf     acc1, W
    selbank selftest_temp
    addwf   selftest_temp, F

    ;; if (adc_temp_value + SELFTEST_TEMPADC_DEV <= selftest_temp) goto t_not_ok
    pagesel t_not_ok
    selbank adc_temp_value
    movf    adc_temp_value, W
    addlw   SELFTEST_TEMPADC_DEV
    selbank selftest_temp
    subwf   selftest_temp, W
    btfsc   STATUS, C
    goto    t_not_ok

    ;; if (adc_temp_value => selftest_temp + SELFTEST_TEMPADC_DEV) goto t_not_ok
    movf    selftest_temp, W
    addlw   SELFTEST_TEMPADC_DEV
    selbank adc_temp_value
    subwf   adc_temp_value, W
    btfsc   STATUS, C
    goto    t_not_ok

    ;; if ((int) timer_value % 256 - (selftest_start_time % 256 + SELFTEST_TEMPADC_DURATION) >= 0) selftest_state |= 1 << SELFTEST_S_TEMPADC
    selbank selftest_start_time
    movf    selftest_start_time, W
    selbank timer_value
    subwf   timer_value, W
    sublw   SELFTEST_TEMPADC_DURATION
    andlw   0x80
    selbank selftest_state + SELFTEST_S_TEMPADC / 8
    btfsc   STATUS, Z
    bsf     selftest_state + SELFTEST_S_TEMPADC / 8, SELFTEST_S_TEMPADC % 8

    pagesel t_end
    goto    t_end

t_not_ok:
    ;; selftest_start_time = timer_value
    mov16ff timer_value, selftest_start_time

    selbank adc_temp_value
    movf    adc_temp_value, W
    selbank selftest_temp
    movwf   selftest_temp

t_end:
    endm

    ;; Requires S_TEMPADC. selftest_temp is no longer updated, and is
    ;; used as a baseline to see if the temperature is falling.
selftest_t_temp     macro
    local   t_ok
    local   t_end

    pagesel t_end
    selbank selftest_state + SELFTEST_S_TEMPADC / 8
    btfss   selftest_state + SELFTEST_S_TEMPADC / 8, SELFTEST_S_TEMPADC % 8
    goto    t_end

    selbank selftest_state + SELFTEST_S_TEMP / 8
    btfsc   selftest_state + SELFTEST_S_TEMP / 8, SELFTEST_S_TEMP % 8
    goto    t_end

    ;; if (adc_temp_value < SELFTEST_TEMP_AMB_MAX) goto t_ok
    pagesel t_ok
    movlw   SELFTEST_TEMP_AMB_MAX
    selbank adc_temp_value
    subwf   adc_temp_value, W
    btfss   STATUS, C
    goto    t_ok

    movlw   SELFTEST_TEMP_AIRFLOW
    airpump_setw

    ;; Require a decrease of at least 3 to call it decreasing.
    ;; if (adc_temp_value + 2 > selftest_temp) goto t_end
    pagesel t_end
    selbank adc_temp_value
    movf    adc_temp_value, W
    addlw   2
    selbank selftest_temp
    subwf   selftest_temp, W
    btfss   STATUS, C
    goto    t_end

t_ok:
    clrw
    airpump_setw

    selbank adc_temp_value
    movf    adc_temp_value, W
    selbank selftest_temp
    movwf   selftest_temp

    selbank selftest_state + SELFTEST_S_TEMP / 8
    bsf     selftest_state + SELFTEST_S_TEMP / 8, SELFTEST_S_TEMP % 8
t_end:
    endm

    ;; Requires S_TEMP and S_ZCPERIOD. selftest_temp is used as a baseline to see
    ;; if the temperature is rising.
selftest_t_heater     macro
    local   t_ok
    local   t_end

    pagesel t_end
    selbank selftest_state + SELFTEST_S_TEMP / 8
    btfss   selftest_state + SELFTEST_S_TEMP / 8, SELFTEST_S_TEMP % 8
    goto    t_end

    selbank selftest_state + SELFTEST_S_ZCPERIOD / 8
    btfss   selftest_state + SELFTEST_S_ZCPERIOD / 8, SELFTEST_S_ZCPERIOD % 8
    goto    t_end

    selbank selftest_state + SELFTEST_S_HEATER / 8
    btfsc   selftest_state + SELFTEST_S_HEATER / 8, SELFTEST_S_HEATER % 8
    goto    t_end

    movlw   SELFTEST_HEATER_AIRFLOW
    airpump_setw

    selbank selftest_temp
    movf    selftest_temp, W
    addlw   SELFTEST_HEATER_TEMP_INC
    heater_setw

    ;; Require an increase of at least SELFTEST_HEATER_TEMP_INC to call it increasing.
    ;; if (adc_temp_value < selftest_temp + SELFTEST_HEATER_TEMP_INC - 1) goto t_end
    selbank selftest_temp
    movf    selftest_temp, W
    addlw   SELFTEST_HEATER_TEMP_INC - 1
    selbank adc_temp_value
    subwf   adc_temp_value, W
    btfss   STATUS, C
    goto    t_end

t_ok:
    ;; New baseline for subsequent tests.
    ;; selftest_temp = adc_temp_value
    movf    adc_temp_value, W
    selbank selftest_temp
    movwf   selftest_temp

    clrw
    heater_setw
    clrw
    airpump_setw

    selbank selftest_state + SELFTEST_S_HEATER / 8
    bsf     selftest_state + SELFTEST_S_HEATER / 8, SELFTEST_S_HEATER % 8
t_end:
    endm

    ;; Requires S_HEATER. selftest_temp is used as a baseline to see
    ;; if the temperature is rising.
selftest_t_airpump     macro
    local   t_ok
    local   t_end

    pagesel t_end
    selbank selftest_state + SELFTEST_S_HEATER / 8
    btfss   selftest_state + SELFTEST_S_HEATER / 8, SELFTEST_S_HEATER % 8
    goto    t_end

    selbank selftest_state + SELFTEST_S_AIRPUMP / 8
    btfsc   selftest_state + SELFTEST_S_AIRPUMP / 8, SELFTEST_S_AIRPUMP % 8
    goto    t_end

    movlw   SELFTEST_AIRPUMP_AIRFLOW
    airpump_setw

    ;; if (adc_temp_value >= selftest_temp) goto t_end
    selbank selftest_temp
    movf    selftest_temp, W
    selbank adc_temp_value
    subwf   adc_temp_value, W
    btfsc   STATUS, C
    goto    t_end

t_ok:
    clrw
    heater_setw
    clrw
    airpump_setw

    selbank selftest_state + SELFTEST_S_AIRPUMP / 8
    bsf     selftest_state + SELFTEST_S_AIRPUMP / 8, SELFTEST_S_AIRPUMP % 8

t_end:
    endm

selftest_idle_func:
    ifndef SELFTEST_ACCEPT_STATE
    error "SELFTEST_ACCEPT_STATE is not defined"
    endif
    ifndef SELFTEST_TIMEOUT
    error "SELFTEST_TIMEOUT is not defined"
    endif

    ;; if (selftest_state & SELFTEST_FAILURES) return
    selbank selftest_state + 1
    movf    selftest_state + 1, W
    andlw   HIGH SELFTEST_FAILURES
    btfss   STATUS, Z
    return

    ;; if ((int) timer_value / 256 - (selftest_start_time / 256 + SELFTEST_HEATER_TIMEOUT) >= 0) goto selftest_idle_timeout
    ;;
    ;; Note that some tests keep incrementing start_time. The timeout is mostly
    ;; useful for the slow tests.
    if      SELFTEST_TIMEOUT > 127
    error   "Assertion failed: SELFTEST_TIMEOUT < 2^7"
    endif
    pagesel selftest_idle_timeout
    selbank selftest_start_time + 1
    movf    selftest_start_time + 1, W
    addlw   SELFTEST_TIMEOUT
    selbank timer_value + 1
    subwf   timer_value + 1, W
    andlw   0x80
    btfsc   STATUS, Z
    goto    selftest_idle_timeout

    selftest_t_power    timer_value
    selftest_t_btnscan2
    selftest_t_btnscan0
    selftest_t_buttons
    selftest_t_switches timer_value
    selftest_t_zcperiod timer_value

    if SELFTEST_ACCEPT_STATE & (1 << SELFTEST_S_TEMPADC)
    selftest_t_tempadc  timer_value
    selftest_t_temp

    ;; Slow tests.
    if SELFTEST_ACCEPT_STATE & (1 << SELFTEST_S_HEATER)
    selftest_t_heater
    if SELFTEST_ACCEPT_STATE & (1 << SELFTEST_S_AIRPUMP)
    selftest_t_airpump
    endif
    endif
    endif

    ;; if ((~selftest_state & SELFTEST_ACCEPT_STATE) == 0) selftest_state.passed = true
    pagesel selftest_not_passed
    selbank selftest_state
    movf    selftest_state, W
    xorlw   0xFF
    andlw   LOW SELFTEST_ACCEPT_STATE
    btfss   STATUS, Z
    goto    selftest_not_passed

    selbank selftest_state + 1
    movf    selftest_state + 1, W
    xorlw   0xFF
    andlw   HIGH SELFTEST_ACCEPT_STATE
    btfss   STATUS, Z
    goto    selftest_not_passed

    selbank             selftest_state + SELFTEST_S_PASSED / 8
    bsf                 selftest_state + SELFTEST_S_PASSED / 8, SELFTEST_S_PASSED % 8
    tempc_set_active    1
    uc_load             0

selftest_not_passed:
    return

selftest_idle_timeout:
    selbank selftest_state + SELFTEST_S_ETIMEOUT / 8
    bsf     selftest_state + SELFTEST_S_ETIMEOUT / 8, SELFTEST_S_ETIMEOUT % 8
    clrw
    heater_setw
    return

selftest_skip_if_passed macro
    selbank selftest_state + SELFTEST_S_PASSED / 8
    btfss   selftest_state + SELFTEST_S_PASSED / 8, SELFTEST_S_PASSED % 8
    endm

selftest_skip_if_not_passed macro
    selbank selftest_state + SELFTEST_S_PASSED / 8
    btfsc   selftest_state + SELFTEST_S_PASSED / 8, SELFTEST_S_PASSED % 8
    endm

selftest_skip_if_failed macro
    selbank selftest_state + 1
    movf    selftest_state + 1, W
    andlw   HIGH SELFTEST_FAILURES
    btfsc   STATUS, Z
    endm

selftest_skip_if_not_failed macro
    selbank selftest_state + 1
    movf    selftest_state + 1, W
    andlw   HIGH SELFTEST_FAILURES
    btfss   STATUS, Z
    endm
    endif

    ifdef section_init
    clr16f  selftest_start_time
    clr16f  selftest_state
    selbank selftest_switches
    clrf    selftest_switches
    selbank selftest_temp
    clrf    selftest_temp
    endif

    ifdef section_idle
    pagesel selftest_idle_func
    selftest_skip_if_passed
    call    selftest_idle_func
    endif
