    ;; ADC module.
    ;;
    ;; RA0 / AN0 - Temperature
    ;; RA3 / AN3 - Knob
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
adc_i           res 1
adc_state		res 1
adc_temp_value  res 1
adc_knob_value  res 1
    endif

    ifdef section_code
adc_idle macro
    local   next_end

    movlw   HIGH next_end
    movwf   PCLATH
    btfsc   adc_state, 0
    goto    next_end

    btfsc   ADCON0, GO
    goto    next_end

    movlw   HIGH adc_var_table
    movwf   PCLATH
    movf    adc_i, W
    call    adc_var_table
    movwf   FSR

    movf    ADRESH, W
    movwf   INDF

    ;; ++adc_i
    incf    adc_i, F

    ;; if (adc_i >= 2) adc_i = 0
    movlw   2
    subwf   adc_i, W
    btfsc   STATUS, C
    clrf    adc_i

    ;; Set up the next channel.
    movlw   HIGH adc_channel_table
    movwf   PCLATH
    movf    adc_i, W
    call    adc_channel_table
    xorwf   ADCON0, W
    andlw   (15 << CHS0)
    xorwf   ADCON0, F

    bsf     adc_state, 0

next_end:
    endm

adc_timer_next macro
    local   timer_next_end
    movlw   HIGH timer_next_end
    movwf   PCLATH
    btfss   adc_state, 0
    goto    timer_next_end

    ;; Start conversion.
    bsf     ADCON0, GO
    bcf     adc_state, 0

timer_next_end:
    endm

adc_channel_table:
    andlw   0x01
    table_jump  tbl
    dt      0 << CHS0
    dt      3 << CHS0

adc_var_table:
    andlw   0x01
    table_jump  tbl
    dt      adc_temp_value
    dt      adc_knob_value
    endif

    ifdef section_init
    movlw   (1 << ADON) | (0 << CHS0)
    movwf   ADCON0
    banksel ADCON1
    movlw   (0 << ADFM) | (0 << VCFG1) | (0 << VCFG0)
    movwf   ADCON1

    banksel ANSEL
    movlw   (1 << RA0) | (1 << RA3)
    movwf   ANSEL
    clrf    ANSELH
    banksel 0

    clrf    adc_i
    clrf    adc_state
    endif

    ifdef section_idle
    adc_idle
    endif