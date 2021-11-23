    ;; LED display high-level module.

    ifdef module_display
#undefine module_display
    endif
#define module_display
    ifndef module_led
    error "led module is a dependency of display"
    endif

    ifdef section_udata
display_i           res 1
display_buf         res 7
display_digitbuf    res 2
    endif

    ifdef section_code
display_timer_next  macro
    ledca_blank

    movlw   display_buf
    movwf   FSR
    movf    display_i, W
    addwf   FSR, F
    movf    INDF, W
    ledseg_set
    ledca_setf_irq display_i

    ;; ++display_i
    incf    display_i, F

    ;; if (display_i >= 7) display_i = 0
    movlw   7
    subwf   display_i, W
    btfsc   STATUS, C
    clrf    display_i
    endm

display_set_digits  macro   base, vf
    local           blank_end

    divmod1608fl    vf, 10
    movwf           acc1
    ledseg_translate acc1
    movwf           base
    divmod1608fl    vf, 10
    movwf           display_digitbuf
    divmod1608fl    vf, 10
    movwf           display_digitbuf + 1

    ;; Blank leading zeros.
    movlw           HIGH blank_end
    movwf           PCLATH
    movf            display_digitbuf + 1, F
    btfss           STATUS, Z
    goto            blank_end

    movlw           0x0F
    movwf           display_digitbuf + 1

    movf            display_digitbuf, F
    btfss           STATUS, Z
    goto            blank_end

    movwf           display_digitbuf
blank_end:

    ledseg_translate display_digitbuf
    movwf           base + 1
    ledseg_translate display_digitbuf + 1
    movwf           base + 2

    endm

display_set_temp    macro   vf
    display_set_digits  display_buf + 0, vf
    endm

display_set_temp_w16        macro
    movlw   HIGH display_set_temp_w16_func
    movwf   PCLATH
    call    display_set_temp_w16_func
    endm

display_set_temp_unit       macro
    movwf   display_buf + 6
    endm

display_set_air     macro   vf
    display_set_digits  display_buf + 3, vf
    endm

display_set_air_w16         macro
    movlw   HIGH display_set_air_w16_func
    movwf   PCLATH
    call    display_set_air_w16_func
    endm

display_set_temp_w16_func:
    display_set_temp    w16
    retlw   0

display_set_air_w16_func:
    display_set_air     w16
    retlw   0
    endif

    ifdef section_init
    clrf    display_i
    movlw   0xFF
    movwf   display_buf
    movwf   display_buf + 1
    movwf   display_buf + 2
    movwf   display_buf + 3
    movwf   display_buf + 4
    movwf   display_buf + 5
    movwf   display_buf + 6
    endif
