    ;; Timer0 module.

    ifdef module_timer0
#undefine module_timer0
    endif
#define module_timer0
    ifndef module_adc
    error "adc module is a dependency of timer0"
    endif
    ifndef module_airpump
    error "airpump module is a dependency of timer0"
    endif
    ifndef module_display
    error "display module is a dependency of timer0"
    endif
    ifndef module_heater
    error "heater module is a dependency of timer0"
    endif

    ifdef section_udata
timer_value res 1
    endif

    ifdef section_code
timer0_irq  macro
    local   irq_end

    movlw   HIGH irq_end
    movwf   PCLATH
    btfss   INTCON, T0IF
    goto    irq_end

    bcf     INTCON, T0IF
    movlw   256 - 156           ; PS=3: 0.0004992 ms, 2003 Hz
    addwf   TMR0, F

    incf    timer_value, F

    display_timer_next
    adc_timer_next
    airpump_timer_next  timer_value
    heater_timer_next   timer_value

irq_end:
    endm
    endif

    ifdef section_init
    banksel OPTION_REG
    movf    OPTION_REG, W
    andlw   ~((1 << PSA) | (1 << T0CS) | (0x07 << PS0))
    iorlw   3 << PS0
    movwf   OPTION_REG
    banksel 0

    bsf     INTCON, T0IE
    endif

    ifdef section_irq
    timer0_irq
    endif
