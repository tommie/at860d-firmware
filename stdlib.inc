    ;; Module for utility functions.

    ifdef module_stdlib
#undefine module_stdlib
    endif
#define module_stdlib

    ifdef section_udata
w16     res 2
acc1    res 1
acc2    res 1
acc3    res 1
tbl     res 1
tblirq  res 1
    endif

    ifdef section_code
table_jump  macro   tblf
    local   case0

    movwf   tblf
    addlw   LOW case0
    movlw   HIGH case0
    btfsc   STATUS, C
    addlw   1
    movwf   PCLATH
    movf    tblf, W
    addwf   PCL, F
case0:
    endm

    ;; w16 *= file
mulw16f macro   file
    local   mulloop
    local   mulend

    clrf    acc1
    clrf    acc2

    movlw   HIGH mulend
    movwf   PCLATH
    movf    file, F
    btfsc   STATUS, Z
    goto    mulend

    movf    file, W
    movwf   acc3

    movlw   HIGH mulloop
    movwf   PCLATH

mulloop:
    movf    w16, W
    addwf   acc1, F
    btfsc   STATUS, C
    incf    acc2, F

    decfsz  acc3, F
    goto    mulloop

mulend:
    movf    acc1, W
    movwf   w16
    movf    acc2, W
    movwf   w16 + 1
    endm

mov16lw macro   l
    movlw   l & 0xFF
    movwf   w16
    movlw   l >> 8
    movwf   w16 + 1
    endm

    ;; W = file16 % l
    ;; file16 /= l
divmod1608fl    macro   file16, l
    local   divloop
    local   divend
    if      file16 == acc1
    error   "Cannot use acc1 for divmod16"
    endif
    movf    file16, W
    movwf   acc1
    movf    file16 + 1, W
    movwf   acc2
    clrf    file16
    clrf    file16 + 1
divloop:
    movlw   l
    subwf   acc1, F
    clrw
    btfss   STATUS, C
    movlw   1
    subwf   acc2, F
    movlw   HIGH divend
    movwf   PCLATH
    btfss   STATUS, C
    goto    divend

    incf    file16, F
    btfsc   STATUS, Z
    incf    file16 + 1, F
    movlw   HIGH divloop
    movwf   PCLATH
    goto    divloop
divend:
    movlw   l
    addwf   acc1, W
    endm

    ;; Divide file by 2^l.
divp2lf macro   file, l
i   set     0
    while   i < l
i   set     i + 1
    bcf     STATUS, C
    rrf     file, F
    endw
    endm
    endif
