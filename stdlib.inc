    ;; Module for utility functions.

    ifdef module_stdlib
#undefine module_stdlib
    endif
#define module_stdlib

    ifdef section_udata_shr
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

    banksel tblf
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

    ;; if ((int) (*file1 - lit) >= 0) goto dest
bge16lf macro   file1, lit, dest
    local   mend

    movlw   HIGH dest
    movwf   PCLATH
    banksel file1 + 1
    movf    file1 + 1, W
    sublw   HIGH lit
    btfss   STATUS, C
    goto    dest

    movlw   HIGH mend
    movwf   PCLATH
    btfss   STATUS, Z
    goto    mend

    movlw   HIGH dest
    movwf   PCLATH
    movlw   LOW lit
    banksel file1
    subwf   file1, W
    btfsc   STATUS, C
    goto    dest
mend:
    endm

    ;; if ((int) (*file1 - *file2) >= 0) goto dest
bge16ff macro   file1, file2, dest
    local   mend

    movlw   HIGH dest
    movwf   PCLATH
    banksel file1 + 1
    movf    file1 + 1, W
    banksel file2 + 1
    subwf   file2 + 1, W
    btfss   STATUS, C
    goto    dest

    movlw   HIGH mend
    movwf   PCLATH
    btfss   STATUS, Z
    goto    mend

    movlw   HIGH dest
    movwf   PCLATH
    banksel file2
    movf    file2, W
    banksel file1
    subwf   file1, W
    btfsc   STATUS, C
    goto    dest
mend:
    endm

    ;; Performs a left-rotate n times on file, setting low bits to carry.
rl16fl      macro   file, n, carry
    local   i

    banksel file
i   set     0
    while   i < n
i   set     i + 1
    if carry
    bsf     STATUS, C
    else
    bcf     STATUS, C
    endif
    rlf     file + 1, F
    rlf     file, F
    endw
    endm

    ;; Performs a right-rotate n times on file, setting high bits to carry.
rr16fl      macro   file, n, carry
    local   i

    banksel file
i   set     0
    while   i < n
i   set     i + 1
    if carry
    bsf     STATUS, C
    else
    bcf     STATUS, C
    endif
    rrf     file + 1, F
    rrf     file, F
    endw
    endm

clr16f  macro   file
    banksel file
    clrf    file + 1
    clrf    file
    endm

add16ff macro   file, dest
    banksel file + 1
    movf    file + 1, W
    banksel dest + 1
    addwf   dest + 1, F
    banksel file
    movf    file, W
    banksel dest
    addwf   dest, F
    banksel dest + 1
    btfsc   STATUS, C
    incf    dest + 1, F
    endm

    ;; dest -= file
sub16ff macro   file, dest
    banksel file + 1
    movf    file + 1, W
    banksel dest + 1
    subwf   dest + 1, F
    banksel file
    movf    file, W
    banksel dest
    subwf   dest, F
    banksel dest + 1
    btfss   STATUS, C
    decf    dest + 1, F
    endm

    ;; w16 = file * lit
mul16fl macro   file, lit
    local   i, j

    clrf    w16 + 1
    clrf    w16
    banksel file

i   set     0
j   set     lit
    while   i < 16
i   set i + 1

    if j & 1
    movf    file + 1, W
    addwf   w16 + 1, F
    movf    file, W
    addwf   w16, F
    btfsc   STATUS, C
    incf    w16 + 1, F
    endif

    bcf     STATUS, C
    rlf     w16, F
    rlf     w16 + 1, F

j   set     j >> 1
    endw
    endm

    ;; w16 *= file
    ;;
    ;; Clobbers acc1, acc2, acc3.
mulw16f macro   file
    local   mulloop
    local   mulend

    clrf    acc1
    clrf    acc2

    movlw   HIGH mulend
    movwf   PCLATH
    banksel file
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

mov16ff macro   src, dest
    banksel src + 1
    movf    src + 1, W
    banksel dest + 1
    movwf   dest + 1
    banksel src
    movf    src, W
    banksel dest
    movwf   dest
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
    banksel file16
    movf    file16, W
    movwf   acc1
    clrf    file16
    banksel file16 + 1
    movf    file16 + 1, W
    movwf   acc2
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

    banksel file16
    incf    file16, F
    banksel file16 + 1
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
    local   i

    banksel file
i   set     0
    while   i < l
i   set     i + 1
    bcf     STATUS, C
    rrf     file, F
    endw
    endm
    endif
