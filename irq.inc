    ;; IRQ framework module.

    ifdef module_irq
#undefine module_irq
    endif
#define module_irq

    ifdef section_udata_shr
irq_w_save      res 1
irq_status_save res 1
irq_fsr_save    res 1
irq_pclath_save res 1
irq_acc1_save   res 1
    endif

    ifdef section_code
irq_enter   macro
    movwf   irq_w_save
    swapf   STATUS, W
    movwf   irq_status_save
    clrf    STATUS
    movf    FSR, W
    movwf   irq_fsr_save
    movf    PCLATH, W
    movwf   irq_pclath_save
    movf    acc1, W
    movwf   irq_acc1_save
    endm

irq_retfie macro
    movf    irq_acc1_save, W
    movwf   acc1
    movf    irq_pclath_save, W
    movwf   PCLATH
    movf    irq_fsr_save, W
    movwf   FSR
    swapf   irq_status_save, W
    movwf   STATUS
    swapf   irq_w_save, F
    swapf   irq_w_save, W
    retfie
    endm

irq_enable  macro
    movf    INTCON, W
    andlw   ~((1 << T0IF) | (1 << INTF) | (1 << RBIF))
    iorlw   (1 << GIE)
    movwf   INTCON
    endm
    endif
