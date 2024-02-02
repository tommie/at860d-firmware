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
display_cursor      res 1
display_cursor_tmr  res 1       ; In 256ms units.

DISP_CUR_MASK       EQU 0x0F
DISP_CUR_EN         EQU 6
DISP_CUR_ON         EQU 7
    endif

    ifdef section_code
display_timer_next  macro
    ledca_blank

    selbank display_buf
    movlw   display_buf
    movwf   FSR
    selbnkc display_i
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

display_set_digits  macro   base, vf, ndigits
    local           blank_end

    selbank         base - 1

i   set             0
    while           i < ndigits
    selbnkd         vf, base + i - 1
    divmod1608fl    vf, 10
    movwf           acc1

    if              i > 0
    ;; if (vf == 0 && acc1 == 0) acc1 = LEDSEG_BLANK
    zero16f         vf
    movlw           LED_BLANK
    btfss           STATUS, Z
    movf            acc1, W
    movf            acc1, F
    btfss           STATUS, Z
    movf            acc1, W
    movwf           acc1
    endif

    ledseg_translate acc1
    selbnkc         base + i
    movwf           base + i
i   set             i + 1
    endw

    display_cur_update
    endm

display_set_temp    macro   vf
    display_set_digits  display_buf + 3, vf, 3
    endm

display_set_temp_w16        macro
    movlw   HIGH display_set_temp_w16_func
    movwf   PCLATH
    call    display_set_temp_w16_func
    endm

display_set_temp_unit       macro
    selbank display_buf + 6
    movwf   display_buf + 6
    endm

display_set_air     macro   vf
    display_set_digits  display_buf + 0, vf, 3
    endm

display_set_air_w16         macro
    movlw   HIGH display_set_air_w16_func
    movwf   PCLATH
    call    display_set_air_w16_func
    endm

    ;; Sets all digits, across air and temp (not temp unit).
    ;;
    ;; Allows displaying numbers larger than 999.
display_set_all    macro   vf
    display_set_digits  display_buf + 0, vf, 6
    endm

display_set_all_w16        macro
    movlw   HIGH display_set_all_w16_func
    movwf   PCLATH
    call    display_set_all_w16_func
    endm

display_set_cursor  macro   digit
    selbank display_cursor
    movlw   (1 << DISP_CUR_EN) | digit
    ;; Avoid irregular flickering if the cursor is already on.
    btfsc   display_cursor, DISP_CUR_ON
    iorlw   1 << DISP_CUR_ON
    movwf   display_cursor
    endm

display_clear_cursor    macro
    local   i

    selbank display_cursor
    clrf    display_cursor
    movlw   ~LSDP
i   set     0
    while   i < 6
    selbnkc display_buf + i
    iorwf   display_buf + i, F
i   set     i + 1
    endw
    endm

display_idle    macro
    local   exit

    movlw   HIGH exit
    movwf   PCLATH
    selbank timer_value + 1
    movf    timer_value + 1, W
    selbnkc display_cursor_tmr
    subwf   display_cursor_tmr, W
    btfss   STATUS, Z
    goto    exit

    ;; We let the timer run even if the cursor is off, to keep a
    ;; steady pace.
    movlw   DISPLAY_CURSOR_IVAL
    selbnkc display_cursor_tmr
    addwf   display_cursor_tmr, F

    selbnkc display_cursor
    btfss   display_cursor, DISP_CUR_EN
    goto    exit

    movlw   1 << DISP_CUR_ON
    selbnkc display_cursor
    xorwf   display_cursor, F

    display_cur_update
exit:
    endm

display_cur_update  macro
    selbank display_cursor
    movf    display_cursor, W
    andlw   DISP_CUR_MASK
    addlw   display_buf
    movwf   FSR

    selbnkc display_cursor
    movlw   LSDP
    btfsc   display_cursor, DISP_CUR_ON
    andwf   INDF, F
    movlw   ~LSDP
    btfss   display_cursor, DISP_CUR_ON
    iorwf   INDF, F
    endm

display_init    macro
    local   i

    selbank display_i
    clrf    display_i
    movlw   0xFF
i   set     0
    while   i < 7
    selbnkc display_buf + i
    movwf   display_buf + i
i   set     i + 1
    endw
    selbnkc display_cursor
    clrf    display_cursor
    selbnkc display_cursor_tmr
    clrf    display_cursor_tmr
    endm

display_set_temp_w16_func:
    display_set_temp    w16
    retlw   0

display_set_air_w16_func:
    display_set_air     w16
    retlw   0

display_set_all_w16_func:
    display_set_all     w16
    retlw   0
    endif

    ifdef section_init
    display_init
    endif

    ifdef section_idle
    display_idle
    endif
