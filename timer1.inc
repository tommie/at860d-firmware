    ;; Timer1 module.
    ;;
    ;; It is synchronized to zerocross, to avoid timing missmatches
	;; causing triac_pfc to pulsate.
    ;;
    ;; We run it at 250 µs to give triac_pfc higher resolution.

    ifdef module_timer1
#undefine module_timer1
    endif
#define module_timer1
    ifndef module_airpump
    error "airpump module is a dependency of timer1"
    endif
    ifndef module_heater
    error "heater module is a dependency of timer1"
    endif

    ifdef section_udata
timer1_value    res 2                   ; Units of 250 µs
TIMER1_RELOAD   equ -(250*20/(4*2) - 2) ; Prescaler 1:2, Fosc=20 MHz -> 250 µs
    endif

    ifdef section_code
timer1_int_next macro
    movlw       HIGH TIMER1_RELOAD
    selbank     TMR1H
    movwf       TMR1H
    movlw       LOW TIMER1_RELOAD
    selbank     TMR1L
    movwf       TMR1L

    selbank     PIR1
    bcf         PIR1, TMR1IF
    endm

timer1_irq  macro
    local   irq_end

    pagesel irq_end
    btfss   PIR1, TMR1IF
    goto    irq_end

    selbank PIR1
    bcf     PIR1, TMR1IF

    movlw   HIGH TIMER1_RELOAD
    selbank TMR1H
    addwf   TMR1H, F
    movlw   LOW TIMER1_RELOAD
    selbank TMR1L
    addwf   TMR1L, F
    selbank TMR1H
    btfsc   STATUS, C
    incf    TMR1H, F

    selbank timer1_value
    incf    timer1_value, F
    selbank timer1_value + 1
    btfsc   STATUS, Z
    incf    timer1_value + 1, F

    airpump_timer_next  timer1_value
    heater_timer_next   timer1_value

irq_end:
    endm
    endif

    ifdef section_init
    selbank T1CON
    movlw   (1 << T1CKPS0) | (0 << TMR1CS) | (1 << TMR1ON) ; 1:2 prescaler
    movwf   T1CON

    selbank INTCON
    bsf     INTCON, PEIE
    selbank PIE1
    bsf     PIE1, TMR1IE
    selbank TMR1H
    clrf    TMR1H
    selbank TMR1L
    clrf    TMR1L
    bcf     PIR1, TMR1IF

    clr16f  timer1_value
    endif

    ifdef section_irq
    timer1_irq
    endif
