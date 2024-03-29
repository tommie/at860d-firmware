    ;; Button matrix scan module.
    ;;
    ;; RB5-7 - Driver output
    ;; RB2-4 - Scan input

    ifdef module_buttonscan
#undefine module_buttonscan
    endif
#define module_buttonscan
    ifndef module_stdlib
    error "stdlib module is a dependency of buttonscan"
    endif

    ifdef section_udata
buttonscan_i        res 1
buttonscan_current  res 2
buttonscan_buf      res 2
    endif

    ifdef section_code
buttonscan_idle     macro
    local   done_newscan

    ;; buttonscan_buf <<= 3
    selbank buttonscan_buf
    rlf     buttonscan_buf, F
    selbank buttonscan_buf + 1
    rlf     buttonscan_buf + 1, F
    selbank buttonscan_buf
    rlf     buttonscan_buf, F
    selbank buttonscan_buf + 1
    rlf     buttonscan_buf + 1, F
    selbank buttonscan_buf
    rlf     buttonscan_buf, F
    selbank buttonscan_buf + 1
    rlf     buttonscan_buf + 1, F
    movlw   ~0x07
    selbank buttonscan_buf
    andwf   buttonscan_buf, F

    ;; buttonscan_buf |= (PORTB >> 2) & 0x07
    selbank PORTB
    comf    PORTB, W            ; Active-low inputs.
    movwf   acc1
    rrf     acc1, F
    rrf     acc1, W
    andlw   0x07
    selbank buttonscan_buf
    iorwf   buttonscan_buf, F

    ;; ++buttonscan_i
    selbank buttonscan_i
    incf    buttonscan_i, F
    ;; if (buttonscan_i >= 3)
    ;;   buttonscan_i = 0
    ;;   buttonscan_current = buttonscan_buf
    ;;   buttonscan_buf = 0
    pagesel done_newscan
    movlw   3
    subwf   buttonscan_i, W
    btfss   STATUS, C
    goto    done_newscan

    clrf    buttonscan_i
    mov16ff buttonscan_buf, buttonscan_current
    clr16f  buttonscan_buf
done_newscan:

    ;; Set PORTB driver pin.
    pagesel buttonscan_driver_table
    selbank buttonscan_i
    movf    buttonscan_i, W
    call    buttonscan_driver_table
    selbank PORTB
    xorwf   PORTB, W
    andlw   (1 << RB5) | (1 << RB6) | (1 << RB7)
    xorwf   PORTB, F
    endm

buttonscan_driver_table:
    andlw       0x03
    table_jump  tbl
    dt          ~(1 << RB5)     ; K3
    dt          ~(1 << RB6)     ; K4, K5, K6
    dt          ~(1 << RB7)     ; handle, K1, K2
    dt          ~0

    endif ; section_code

    ifdef section_init
    movlw   0xFC                ; Outputs are active low. Inputs have pull-ups.
    selbank PORTB
    iorwf   PORTB, F
    selbank TRISB
    movlw   0x1F
    xorwf   TRISB, W
    andlw   0xFC
    xorwf   TRISB, F

    selbank OPTION_REG
    bcf     OPTION_REG, NOT_RBPU
    movlw   0xFC
    selbank WPUB
    movwf   WPUB

    selbank buttonscan_i
    clrf    buttonscan_i
    clr16f  buttonscan_buf
    clr16f  buttonscan_current
    endif

    ifdef section_idle
    buttonscan_idle
    endif
