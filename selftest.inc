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

    ;; 100 * 500 µs is 50 ms.
SELFTEST_SWITCHES_DURATION  equ 100
    ;; Two periods at 2*50 Hz is 20 ms. 50 ms means four.
SELFTEST_ZCPERIOD_DURATION  equ 100
SELFTEST_ZCPERIOD_MIN       equ 16 - 5
SELFTEST_ZCPERIOD_MAX       equ 20 + 5
SELFTEST_TEMPADC_DURATION   equ 100
SELFTEST_TEMPADC_DEV        equ 1
SELFTEST_TEMP_AMB_MAX       equ 10
SELFTEST_HEATER_AIRFLOW     equ 128
SELFTEST_HEATER_TEMP_INC    equ 5
SELFTEST_AIRPUMP_AIRFLOW    equ 128
    endif

    ifdef section_code
selftest_t_power    macro   timer_value
    local       t_end

    movlw       HIGH t_end
    movwf       PCLATH
    btfsc       selftest_state + SELFTEST_S_POWER / 8, SELFTEST_S_POWER % 8
    goto        t_end

    swpower_get selftest_state + SELFTEST_S_POWER / 8, SELFTEST_S_POWER % 8

    movf        timer_value, W
    movwf       selftest_start_time
    movf        timer_value + 1, W
    movwf       selftest_start_time + 1
t_end:
    endm

selftest_t_btnscan2 macro
    local   t_end

    movlw   HIGH t_end
    movwf   PCLATH
    btfsc   selftest_state + SELFTEST_S_BTNSCAN2 / 8, SELFTEST_S_BTNSCAN2 % 8
    goto    t_end

    movlw   2
    subwf   buttonscan_i, W
    btfsc   STATUS, Z
    bsf     selftest_state + SELFTEST_S_BTNSCAN2 / 8, SELFTEST_S_BTNSCAN2 % 8
t_end:
    endm

    ;; Requires S_BTNSCAN2.
selftest_t_btnscan0 macro
    local   t_end

    movlw   HIGH t_end
    movwf   PCLATH
    btfss   selftest_state + SELFTEST_S_BTNSCAN2 / 8, SELFTEST_S_BTNSCAN2 % 8
    goto    t_end

    btfsc   selftest_state + SELFTEST_S_BTNSCAN0 / 8, SELFTEST_S_BTNSCAN0 % 8
    goto    t_end

    movf    buttonscan_i, F
    btfsc   STATUS, Z
    bsf     selftest_state + SELFTEST_S_BTNSCAN0 / 8, SELFTEST_S_BTNSCAN0 % 8
t_end:
    endm


    ;; Requires S_BTNSCAN0.
selftest_t_buttons macro
    local   t_end

    movlw   HIGH t_end
    movwf   PCLATH
    btfss   selftest_state + SELFTEST_S_BTNSCAN0 / 8, SELFTEST_S_BTNSCAN0 % 8
    goto    t_end

    btfsc   selftest_state + SELFTEST_S_BUTTONS / 8, SELFTEST_S_BUTTONS % 8
    goto    t_end

    movf    buttonscan_current, F
    btfss   STATUS, Z
    goto    t_end
    movf    buttonscan_current + 1, F
    btfsc   STATUS, Z
    bsf     selftest_state + SELFTEST_S_BUTTONS / 8, SELFTEST_S_BUTTONS % 8
t_end:
    endm

selftest_get_switches   macro   file, first_bit
    clrf            file
    swcal_get       file, first_bit + 0
    swset_get       file, first_bit + 1
    swstandby_get   file, first_bit + 2
    swpower_get     file, first_bit + 3
    endm


selftest_t_switches macro   timer_value
    local           t_ok
    local           t_end

    movlw           HIGH t_end
    movwf           PCLATH
    btfsc           selftest_state + SELFTEST_S_SWITCHES / 8, SELFTEST_S_SWITCHES % 8
    goto            t_end

    selftest_get_switches   acc1, 4

    ;; Record if any switch has ever flickered.
    ;; selftest_switches = ((((selftest_switches ^ acc1) >> 4) | selftest_switches) & 0x0F) | acc1
    movf            acc1, W
    xorwf           selftest_switches, F
    swapf           selftest_switches, W
    iorwf           selftest_switches, W
    andlw           0x0F
    iorwf           acc1, W
    movwf           selftest_switches

    ;; if (timer_value - selftest_start_time <= SELFTEST_SWITCHES_DURATION) goto t_end
    movf            selftest_start_time, W
    subwf           timer_value, W
    sublw           SELFTEST_SWITCHES_DURATION
    btfsc           STATUS, C
    goto            t_end

    ;; if ((selftest_switches & 0x0F) == 0) goto t_ok
    movlw           0x0F
    andwf           selftest_switches, W
    movlw           HIGH t_ok
    movwf           PCLATH
    btfsc           STATUS, Z
    goto            t_ok

    ;; selftest_switches = acc1
    movf            acc1, W
    movwf           selftest_switches

    ;; selftest_start_time = timer_value
    movf            timer_value, W
    movwf           selftest_start_time
    movf            timer_value + 1, W
    movwf           selftest_start_time + 1

    movlw           HIGH t_end
    movwf           PCLATH
    goto            t_end

t_ok:
    bsf             selftest_state + SELFTEST_S_SWITCHES / 8, SELFTEST_S_SWITCHES % 8

t_end:
    endm

    ;; This resets selftest_start_time until the period is reasonable.
selftest_t_zcperiod macro   timer_value
    local   t_end

    movlw   HIGH t_end
    movwf   PCLATH
    btfsc   selftest_state + SELFTEST_S_ZCPERIOD / 8, SELFTEST_S_ZCPERIOD % 8
    goto    t_end

    ;; if (zero_cross_period <= SELFTEST_ZCPERIOD_MIN) selftest_start_time = timer_value
    movf    zero_cross_period, W
    sublw   SELFTEST_ZCPERIOD_MIN
    movf    timer_value, W
    btfsc   STATUS, C
    movwf   selftest_start_time
    movf    timer_value + 1, W
    btfsc   STATUS, C
    movwf   selftest_start_time + 1

    ;; if (zero_cross_period > SELFTEST_ZCPERIOD_MAX) selftest_start_time = timer_value
    movf    zero_cross_period, W
    sublw   SELFTEST_ZCPERIOD_MAX
    movf    timer_value, W
    btfss   STATUS, C
    movwf   selftest_start_time
    movf    timer_value + 1, W
    btfss   STATUS, C
    movwf   selftest_start_time + 1

    ;; if (timer_value - selftest_start_time <= SELFTEST_ZCPERIOD_DURATION) goto t_end
    movf    selftest_start_time, W
    subwf   timer_value, W
    sublw   SELFTEST_ZCPERIOD_DURATION
    btfsc   STATUS, C
    goto    t_end

    bsf     selftest_state + SELFTEST_S_ZCPERIOD / 8, SELFTEST_S_ZCPERIOD % 8
t_end:
    endm

selftest_t_tempadc  macro   timer_value
    local   t_not_ok
    local   t_end

    movlw   HIGH t_end
    movwf   PCLATH
    btfsc   selftest_state + SELFTEST_S_TEMPADC / 8, SELFTEST_S_TEMPADC % 8
    goto    t_end

    ;; selftest_temp += (adc_temp_value - selftest_temp +- 4) / 8
    movf    selftest_temp, W
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
    bcf     STATUS, C
    btfsc   acc1, 7
    bsf     STATUS, C
    rrf     acc1, W
    addwf   selftest_temp, F

    ;; if (adc_temp_value + SELFTEST_TEMPADC_DEV < selftest_temp) goto t_not_ok
    movlw   HIGH t_not_ok
    movwf   PCLATH
    movf    adc_temp_value, W
    addlw   SELFTEST_TEMPADC_DEV + 1
    subwf   selftest_temp, W
    btfsc   STATUS, C
    goto    t_not_ok

    ;; if (adc_temp_value > selftest_temp + SELFTEST_TEMPADC_DEV) goto t_not_ok
    movf    selftest_temp, W
    addlw   SELFTEST_TEMPADC_DEV + 1
    subwf   adc_temp_value, W
    btfsc   STATUS, C
    goto    t_not_ok

    ;; if (timer_value - selftest_start_time > SELFTEST_TEMPADC_DURATION) selftest_state |= 1 << SELFTEST_S_TEMPADC
    movf    selftest_start_time, W
    subwf   timer_value, W
    sublw   SELFTEST_TEMPADC_DURATION
    btfss   STATUS, C
    bsf     selftest_state + SELFTEST_S_TEMPADC / 8, SELFTEST_S_TEMPADC % 8

    movlw   HIGH t_end
    movwf   PCLATH
    goto    t_end

t_not_ok:
    ;; selftest_start_time = timer_value
    movf    timer_value, W
    movwf   selftest_start_time
    movf    timer_value + 1, W
    movwf   selftest_start_time + 1

t_end:
    endm

    ;; Requires S_TEMPADC. selftest_temp is no longer updated, and is
    ;; used as a baseline to see if the temperature is falling.
selftest_t_temp     macro
    local   t_ok
    local   t_end

    movlw   HIGH t_end
    movwf   PCLATH
    btfss   selftest_state + SELFTEST_S_TEMPADC / 8, SELFTEST_S_TEMPADC % 8
    goto    t_end

    btfsc   selftest_state + SELFTEST_S_TEMP / 8, SELFTEST_S_TEMP % 8
    goto    t_end

    ;; if (adc_temp_value < SELFTEST_TEMP_AMB_MAX) goto t_ok
    movlw   HIGH t_ok
    movwf   PCLATH
    movlw   SELFTEST_TEMP_AMB_MAX
    subwf   adc_temp_value, W
    btfss   STATUS, C
    goto    t_ok

    ;; Require a decrease of at least 3 to call it decreasing.
    ;; if (adc_temp_value + 2 > selftest_temp) goto t_end
    movlw   HIGH t_end
    movwf   PCLATH
    movf    adc_temp_value, W
    addlw   2
    subwf   selftest_temp, W
    btfss   STATUS, C
    goto    t_end

t_ok:
    movf    adc_temp_value, W
    movwf   selftest_temp

    bsf     selftest_state + SELFTEST_S_TEMP / 8, SELFTEST_S_TEMP % 8
t_end:
    endm

    ;; Requires S_TEMP. selftest_temp is used as a baseline to see
    ;; if the temperature is rising.
selftest_t_heater     macro
    local   t_ok
    local   t_end

    movlw   HIGH t_end
    movwf   PCLATH
    btfss   selftest_state + SELFTEST_S_TEMP / 8, SELFTEST_S_TEMP % 8
    goto    t_end

    btfsc   selftest_state + SELFTEST_S_HEATER / 8, SELFTEST_S_HEATER % 8
    goto    t_end

    movlw   SELFTEST_HEATER_AIRFLOW
    airpump_setw

    movf    selftest_temp, W
    addlw   SELFTEST_HEATER_TEMP_INC
    heater_setw

    ;; Require an increase of at least SELFTEST_HEATER_TEMP_INC to call it increasing.
    ;; if (adc_temp_value < selftest_temp + SELFTEST_HEATER_TEMP_INC - 1) goto t_end
    movf    selftest_temp, W
    addlw   SELFTEST_HEATER_TEMP_INC - 1
    subwf   adc_temp_value, W
    btfss   STATUS, C
    goto    t_end

t_ok:
    ;; New baseline for subsequent tests.
    ;; selftest_temp = adc_temp_value
    movf    adc_temp_value, W
    movwf   selftest_temp

    clrw
    heater_setw
    clrw
    airpump_setw

    bsf     selftest_state + SELFTEST_S_HEATER / 8, SELFTEST_S_HEATER % 8
t_end:
    endm

    ;; Requires S_HEATER. selftest_temp is used as a baseline to see
    ;; if the temperature is rising.
selftest_t_airpump     macro
    local   t_ok
    local   t_end

    movlw   HIGH t_end
    movwf   PCLATH
    btfss   selftest_state + SELFTEST_S_HEATER / 8, SELFTEST_S_HEATER % 8
    goto    t_end

    btfsc   selftest_state + SELFTEST_S_AIRPUMP / 8, SELFTEST_S_AIRPUMP % 8
    goto    t_end

    movlw   SELFTEST_AIRPUMP_AIRFLOW
    airpump_setw

    ;; if (adc_temp_value >= selftest_temp) goto t_end
    movf    selftest_temp, W
    subwf   adc_temp_value, W
    btfsc   STATUS, C
    goto    t_end

t_ok:
    clrw
    heater_setw
    clrw
    airpump_setw

    bsf     selftest_state + SELFTEST_S_AIRPUMP / 8, SELFTEST_S_AIRPUMP % 8

t_end:
    endm

selftest_idle_func:
    ifndef SELFTEST_TIMEOUT
    error "SELFTEST_TIMEOUT is not defined"
    endif

    ;; if (selftest_state & SELFTEST_FAILURES) return
    movf    selftest_state + 1, W
    andlw   HIGH SELFTEST_FAILURES
    btfss   STATUS, Z
    return

    ;; if (timer_value - selftest_start_time >= SELFTEST_HEATER_TIMEOUT) goto selftest_idle_timeout
    ;; Note that some tests keep incrementing start_time. The timeout is mostly
    ;; useful for the slow tests.
    movlw   HIGH selftest_idle_timeout
    movwf   PCLATH
    movf    selftest_start_time + 1, W
    subwf   timer_value + 1, W
    sublw   SELFTEST_TIMEOUT
    btfss   STATUS, C
    goto    selftest_idle_timeout

    selftest_t_power    timer_value
    selftest_t_btnscan2
    selftest_t_btnscan0
    selftest_t_buttons
    selftest_t_switches timer_value
    selftest_t_zcperiod timer_value
    selftest_t_tempadc  timer_value
    selftest_t_temp

    ;; Slow tests.
    selftest_t_heater
    selftest_t_airpump

    ;; if ((~selftest_state & SELFTEST_ACCEPT_STATE) == 0) selftest_state.passed = true
    movlw   HIGH selftest_not_passed
    movwf   PCLATH
    movf    selftest_state, W
    xorlw   0xFF
    andlw   LOW SELFTEST_ACCEPT_STATE
    btfss   STATUS, Z
    goto    selftest_not_passed

    movf    selftest_state + 1, W
    xorlw   0xFF
    andlw   HIGH SELFTEST_ACCEPT_STATE
    btfsc   STATUS, Z
    bsf     selftest_state + SELFTEST_S_PASSED / 8, SELFTEST_S_PASSED % 8
selftest_not_passed:

    return

selftest_idle_timeout:
    bsf     selftest_state + SELFTEST_S_ETIMEOUT / 8, SELFTEST_S_ETIMEOUT % 8
    clrw
    heater_setw
    return

selftest_skip_if_passed macro
    btfss   selftest_state + SELFTEST_S_PASSED / 8, SELFTEST_S_PASSED % 8
    endm

selftest_goto_if_failed macro   addr
    movf    selftest_state + 1, W
    andlw   HIGH SELFTEST_FAILURES
    movlw   HIGH addr
    movwf   PCLATH
    btfss   STATUS, Z
    goto    addr
    endm

selftest_reset  macro
    clrf    selftest_start_time
    clrf    selftest_start_time + 1
    clrf    selftest_state
    clrf    selftest_state + 1
    clrf    selftest_switches
    clrf    selftest_temp
    endm
    endif

    ifdef section_init
    ifndef SELFTEST_ACCEPT_STATE
    error "SELFTEST_ACCEPT_STATE is not defined"
    endif

    selftest_reset
    endif

    ifdef section_idle
    movlw   HIGH selftest_idle_func
    movwf   PCLATH
    selftest_skip_if_passed
    call    selftest_idle_func
    endif
