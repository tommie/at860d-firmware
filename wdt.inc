    ;; Watchdog timer module.

    ifdef module_wdt
#undefine module_wdt
    endif
#define module_wdt
    ifndef module_led
    error "led module is a dependency of wdt"
    endif

    ifdef section_code
    ;; Show the WDT timeout as "*1" and sleep until reset.
    ;;
    ;; NOT_POR is used to signal that we have already handled the
    ;; WDTO. If MCLR is asserted, we'll be able to tell.
wdto:
    selbnkc PCON
    bsf     PCON, NOT_POR

    movlw   LS2B & LS2C & LS2DP
    ledseg_set
    movlw   6
    movwf   acc1
    ledca_setf_irq acc1
    ;buzzer_set  STATUS, NOT_TO
    movlw   HIGH wdto_loop
    movwf   PCLATH

wdto_loop:
    clrwdt
    sleep
    goto    wdto_loop
    endif

    ifdef section_init
    selbank PCON
    movlw   HIGH not_wdto
    movwf   PCLATH
    btfsc   PCON, NOT_POR
    goto    not_wdto
    movlw   HIGH wdto
    movwf   PCLATH
    btfss   STATUS, NOT_TO
    goto    wdto
not_wdto:

    ;; See comment in wdto.
    bcf     PCON, NOT_POR

    ;; 1.06s WDT
    selbnkc WDTCON
    movlw   (10 << WDTPS0)
    movwf   WDTCON
    endif

    ifdef section_idle
    clrwdt
    endif
