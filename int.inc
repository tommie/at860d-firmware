    ;; External interrupt IRQ module.

    ifdef module_int
#undefine module_int
    endif
#define module_int
    ifndef module_airpump
    error "airpump module is a dependency of timer0"
    endif
    ifndef module_heater
    error "heater module is a dependency of timer0"
    endif
    ifndef module_zerocross
    error "zerocross module is a dependency of int"
    endif

    ifdef section_code
int_irq     macro
    local   int_end

    movlw   HIGH int_end
    movwf   PCLATH
    btfss   INTCON, INTF
    goto    int_end

    bcf     INTCON, INTF
    zero_cross_int_next
    airpump_int_next

int_end:
    endm
    endif

    ifdef section_init
    bsf     INTCON, INTE
    endif

    ifdef section_irq
    int_irq
    endif
