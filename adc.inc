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

    movlw   HIGH next_end
    movwf   PCLATH
    selbank adc_state
    btfsc   adc_state, 0
    goto    next_end

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
    endif

    ;; if ((adc_i & 0x0F) != 0) goto multisample_done
    movlw   HIGH multisample_done
    movwf   PCLATH
    movlw   0x0F
    selbnkc adc_i
    andwf   adc_i, W
    btfss   STATUS, Z
    goto    multisample_done

    ;; adc_acc >>= ADC_MULTISAMPLE_LOG
    rr16fl  adc_acc, ADC_MULTISAMPLE_LOG, 0

    ;; *adc_var_table[adc_i] = adc_acc
    movlw   HIGH adc_var_table
    movwf   PCLATH
    selbnkc adc_i
    swapf   adc_i, W            ; Upper nibble
    addlw   -1                  ; Previous channel
    call    adc_var_table
    movwf   FSR
    bcf     STATUS, IRP         ; TODO: need to set IRP bit correctly

    selbnkc adc_acc
    movf    adc_acc, W
    movwf   INDF
    selbnkc adc_acc + 1
    movf    adc_acc + 1, W
    incf    FSR, F
    movwf   INDF

    ;; adc_acc = 0
    clr16f  adc_acc
multisample_done:

    ;; if (adc_i >= 2 << 4) adc_i = 0
    movlw   2 << 4
    subwf   adc_i, W
    btfsc   STATUS, C
    clrf    adc_i

    ;; Set up the next channel.
    movlw   HIGH adc_channel_table
    movwf   PCLATH
    swapf   adc_i, W            ; Upper nibble
    call    adc_channel_table
    xorwf   ADCON0, W
    andlw   (15 << CHS0)
    xorwf   ADCON0, F

    selbnkc adc_state
    bsf     adc_state, 0

next_end:
    endm

adc_timer_next macro
    local   timer_next_end

    movlw   HIGH timer_next_end
    movwf   PCLATH
    selbank adc_state
    btfss   adc_state, 0
    goto    timer_next_end

    ;; Start conversion.
    bsf     ADCON0, GO
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
    endif

    ifdef section_init
    movlw   (1 << ADON) | (0 << CHS0)
    movwf   ADCON0
    selbank ADCON1
    movlw   (1 << ADFM) | (0 << VCFG1) | (0 << VCFG0)
    movwf   ADCON1

    selbnkc ANSEL
    movlw   (1 << RA0) | (1 << RA3)
    movwf   ANSEL
    clrf    ANSELH

    selbnkc adc_i
    clrf    adc_i
    selbnkc adc_state
    clrf    adc_state
    clr16f  adc_temp_value
    clr16f  adc_knob_value
    clr16f  adc_acc
    endif

    ifdef section_idle
    adc_idle
    endif
