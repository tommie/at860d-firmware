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
buttonscan_prev     res 2
    endif

    ifdef section_code
buttonscan_idle     macro
    local   done_newscan

    ;; buttonscan_buf <<= 3
    rlf     buttonscan_buf, F
    rlf     buttonscan_buf + 1, F
    rlf     buttonscan_buf, F
    rlf     buttonscan_buf + 1, F
    rlf     buttonscan_buf, F
    rlf     buttonscan_buf + 1, F
    movlw   ~0x07
    andwf   buttonscan_buf, F

    ;; buttonscan_buf |= (PORTB >> 2) & 0x07
    comf    PORTB, W            ; Active-low inputs.
    movwf   acc1
    rrf     acc1, F
    rrf     acc1, W
    andlw   0x07
    iorwf   buttonscan_buf, F

    ;; ++buttonscan_i
    incf    buttonscan_i, F
    ;; if (buttonscan_i >= 3)
    ;;   buttonscan_i = 0
    ;;   buttonscan_current = buttonscan_buf
    movlw   HIGH done_newscan
    movwf   PCLATH
    movlw   3
    subwf   buttonscan_i, W
    btfss   STATUS, C
    goto    done_newscan

    clrf    buttonscan_i
    movf    buttonscan_buf, W
    movwf   buttonscan_current
    movf    buttonscan_buf + 1, W
    movwf   buttonscan_current + 1
    clrf    buttonscan_buf
    clrf    buttonscan_buf + 1
done_newscan:

    ;; Set PORTB driver pin.
    movlw   HIGH buttonscan_driver_table
    movwf   PCLATH
    movf    buttonscan_i, W
    call    buttonscan_driver_table
    xorwf   PORTB, W
    andlw   (1 << RB5) | (1 << RB6) | (1 << RB7)
    xorwf   PORTB, F
    endm

buttonscan_driver_table:
    andlw   0x03
    table_jump  tbl
    dt      ~(1 << RB5)         ; K3
    dt      ~(1 << RB6)         ; K4, K5, K6
    dt      ~(1 << RB7)         ; handle, K1, K2
    dt      ~0

buttonscan_get_delta    macro
    movf    buttonscan_current, W
    xorwf   buttonscan_prev, W
    xorwf   buttonscan_prev, F
    andf    buttonscan_current, W
    endm
    endif

    ifdef section_init
    movlw   0xFC                ; Outputs are active low. Inputs have pull-ups.
    iorwf   PORTB, F
    banksel TRISB
    movlw   0x1F
    xorwf   TRISB, W
    andlw   0xFC
    xorwf   TRISB, F

    bcf     OPTION_REG, NOT_RBPU
    movlw   0xFC
    banksel WPUB
    movwf   WPUB
    banksel 0

    clrf    buttonscan_i
    clrf    buttonscan_buf
    clrf    buttonscan_buf + 1
    clrf    buttonscan_current
    clrf    buttonscan_current + 1
    clrf    buttonscan_prev
    clrf    buttonscan_prev + 1
    endif

    ifdef section_idle
    buttonscan_idle
    endif
