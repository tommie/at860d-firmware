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
display_cursor      res 1
display_cursor_tmr  res 1       ; In 256ms units.

DISP_CUR_MASK       EQU 0x0F
DISP_CUR_EN         EQU 6
DISP_CUR_ON         EQU 7
    endif

    ifdef section_code
display_timer_next  macro
    ledca_blank

    banksel display_buf
    movlw   display_buf
    movwf   FSR
    banksel display_i
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

    banksel         vf
    divmod1608fl    vf, 10
    movwf           acc1
    ledseg_translate acc1
    banksel         base
    movwf           base
    banksel         vf
    divmod1608fl    vf, 10
    banksel         display_digitbuf
    movwf           display_digitbuf
    banksel         vf
    divmod1608fl    vf, 10
    banksel         display_digitbuf + 1
    movwf           display_digitbuf + 1

    ;; Blank leading zeros.
    movlw           HIGH blank_end
    movwf           PCLATH
    banksel         display_digitbuf + 1
    movf            display_digitbuf + 1, F
    btfss           STATUS, Z
    goto            blank_end

    movlw           0x0F
    movwf           display_digitbuf + 1

    banksel         display_digitbuf
    movf            display_digitbuf, F
    btfss           STATUS, Z
    goto            blank_end

    movwf           display_digitbuf
blank_end:

    banksel         display_digitbuf
    ledseg_translate display_digitbuf
    banksel         base + 1
    movwf           base + 1
    banksel         display_digitbuf + 1
    ledseg_translate display_digitbuf + 1
    banksel         base + 2
    movwf           base + 2

    display_cur_update
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
    banksel display_buf + 6
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

display_set_cursor  macro   digit
    movlw   (1 << DISP_CUR_ON) | (1 << DISP_CUR_EN) | digit
    banksel display_cursor
    movwf   display_cursor
    endm

display_clear_cursor    macro
    banksel display_cursor
    clrf    display_cursor
    movlw   ~LSDP
i   set     0
    while   i < 6
    banksel display_buf + i
    iorwf   display_buf + i, F
i   set     i + 1
    endw
    endm

display_idle    macro
    local   exit

    movlw   HIGH exit
    movwf   PCLATH
    banksel timer_value + 1
    movf    timer_value + 1, W
    banksel display_cursor_tmr
    subwf   display_cursor_tmr, W
    btfss   STATUS, Z
    goto    exit

    ;; We let the timer run even if the cursor is off, to keep a
    ;; steady pace.
    movlw   DISPLAY_CURSOR_IVAL
    banksel display_cursor_tmr
    addwf   display_cursor_tmr, F

    banksel display_cursor
    btfss   display_cursor, DISP_CUR_EN
    goto    exit

    movlw   1 << DISP_CUR_ON
    banksel display_cursor
    xorwf   display_cursor, F

    display_cur_update
exit:
    endm

display_cur_update  macro
    banksel display_cursor
    movf    display_cursor, W
    andlw   DISP_CUR_MASK
    addlw   display_buf
    movwf   FSR

    banksel display_cursor
    movlw   LSDP
    btfsc   display_cursor, DISP_CUR_ON
    andwf   INDF, F
    movlw   ~LSDP
    btfss   display_cursor, DISP_CUR_ON
    iorwf   INDF, F
    endm

display_set_temp_w16_func:
    display_set_temp    w16
    retlw   0

display_set_air_w16_func:
    display_set_air     w16
    retlw   0
    endif

    ifdef section_init
    banksel display_i
    clrf    display_i
    movlw   0xFF
i   set     0
    while   i < 7
    banksel display_buf + i
    movwf   display_buf + i
i   set     i + 1
    endw
    banksel display_cursor
    clrf    display_cursor
    banksel display_cursor_tmr
    clrf    display_cursor_tmr
    endif

    ifdef section_idle
    display_idle
    endif
