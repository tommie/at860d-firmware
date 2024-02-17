    ;; ADC module.
    ;;
    ;; RA0 / AN0 - Temperature, 10-bits
    ;; RA3 / AN3 - Knob, 10-bits
    ;;
    ;; States:
    ;;   0 - ADC is complete. adc_next sets up CHS for next channel.
    ;;   1 - Channel acquisition time complete. Start ADC.

    ifdef module_adc
#undefine module_adc
    endif
#define module_adc
    ifndef module_stdlib
    error "stdlib module is a dependency of adc"
    endif

    ifdef section_udata
ADC_MULTISAMPLE_LOG equ 4       ; The log_2(number of multisamples) in [0, 4]

adc_i           res 1           ; Lower nibble is for multisampling.
adc_state       res 1
adc_temp_value  res 2
adc_knob_value  res 2
adc_acc         res 2           ; Buffer for multisampling.
    endif

    ifdef section_code
adc_idle macro
    local   multisample_done
    local   next_end

    pagesel next_end
    selbank adc_state
    btfsc   adc_state, 0
    goto    next_end

    selbnkc ADCON0
    btfsc   ADCON0, GO
    goto    next_end

    ;; adc_acc += ADRES
    selbnkc ADRESL
    movf    ADRESL, W
    selbnkc adc_acc
    addwf   adc_acc, F
    selbnkc adc_acc + 1
    btfsc   STATUS, C
    incf    adc_acc + 1, F

    selbnkc ADRESH
    movf    ADRESH, W
    ;; Depending on where you read, the reserved upper bits are documented as
    ;; unknown (Register 9-5) or read as zero (Figure 9-3).
    selbnkc adc_acc + 1
    addwf   adc_acc + 1, F

    ;; ++adc_i
    selbnkc adc_i
    incf    adc_i, F

    ;; If the multisample counter is smaller than a nibble, forward wrap-around.
    if      ADC_MULTISAMPLE_LOG > 4
    error   "ADC_MULTISAMPLE_LOG greater than 4 is not supported"
    endif
    if      ADC_MULTISAMPLE_LOG < 4
    movlw   0x10
    selbnkc adc_i
    btfsc   adc_i, ADC_MULTISAMPLE_LOG
    addwf   adc_i, F
    movlw   ~(((1 << (4 - ADC_MULTISAMPLE_LOG)) - 1) << ADC_MULTISAMPLE_LOG)
    andwf   adc_i, F
    endif   ; ADC_MULTISAMPLE_LOG < 4

    ;; if ((adc_i & 0x0F) != 0) goto multisample_done
    pagesel multisample_done
    movlw   0x0F
    selbnkc adc_i
    andwf   adc_i, W
    btfss   STATUS, Z
    goto    multisample_done

    ;; adc_acc >>= ADC_MULTISAMPLE_LOG
    rr16fl  adc_acc, ADC_MULTISAMPLE_LOG, 0

    ;; *adc_var_table[adc_i] = adc_acc
    pagesel adc_var_table
    selbnkc adc_i
    swapf   adc_i, W            ; Upper nibble
    addlw   -1                  ; Previous channel
    movwf   acc1
    call    adc_var_table
    movwf   FSR
    movf    acc1, W
    pagesel adc_varh_table
    call    adc_varh_table
    bcf     STATUS, IRP
    iorlw   0
    btfss   STATUS, Z
    bsf     STATUS, IRP

    selbank adc_acc
    movf    adc_acc, W
    movwf   INDF
    selbnkc adc_acc + 1
    movf    adc_acc + 1, W
    incf    FSR, F
    btfsc   STATUS, Z
    bsf     STATUS, IRP
    movwf   INDF

    ;; adc_acc = 0
    clr16f  adc_acc
multisample_done:

    ;; if (adc_i >= 2 << 4) adc_i = 0
    movlw   2 << 4
    selbank adc_i
    subwf   adc_i, W
    btfsc   STATUS, C
    clrf    adc_i

    ;; Set up the next channel.
    pagesel adc_channel_table
    swapf   adc_i, W            ; Upper nibble
    call    adc_channel_table
    selbank ADCON0
    xorwf   ADCON0, W
    andlw   (15 << CHS0)
    xorwf   ADCON0, F

    selbnkc adc_state
    bsf     adc_state, 0

next_end:
    endm

adc_timer_next macro
    local   timer_next_end

    pagesel timer_next_end
    selbank adc_state
    btfss   adc_state, 0
    goto    timer_next_end

    ;; Start conversion.
    selbnkc ADCON0
    bsf     ADCON0, GO
    selbnkc adc_state
    bcf     adc_state, 0

timer_next_end:
    endm

adc_channel_table:
    andlw       0x01
    table_jump  tbl
    dt          0 << CHS0
    dt          3 << CHS0

adc_var_table:
    andlw       0x01
    table_jump  tbl
    dt          LOW adc_temp_value
    dt          LOW adc_knob_value

adc_varh_table:
    andlw       0x01
    table_jump  tbl
    dt          HIGH adc_temp_value
    dt          HIGH adc_knob_value
    endif ; section_code

    ifdef section_init
    ;; 1:32 prescaler is the fastest possible at Fosc=20 MHz.
    movlw   (2 << ADCS0) | (1 << ADON) | (0 << CHS0)
    selbank ADCON0
    movwf   ADCON0
    selbnkc ADCON1
    movlw   (1 << ADFM) | (0 << VCFG1) | (0 << VCFG0)
    movwf   ADCON1

    selbnkc ANSEL
    movlw   (1 << RA0) | (1 << RA3)
    movwf   ANSEL
    selbnkc ANSELH
    clrf    ANSELH

    selbnkc adc_i
    clrf    adc_i
    selbnkc adc_state
    clrf    adc_state
    clr16f  adc_temp_value
    clr16f  adc_knob_value
    clr16f  adc_acc
    endif ; section_init

    ifdef section_idle
    adc_idle
    endif
