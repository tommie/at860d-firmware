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

    ;; The bank issued by an earlier selbank, or -1 if unknown.
bank    set -1
    ;; Issues a banksel, and updates the bank variable.
    ;; Ignores bank-agnostic files.
selbank     macro   file
    if      ((file) & 0x7F) < 0x70 ; Files in udata won't be allocated here, and they default to zero, causing this to be safe.
    banksel file
bank    set (file) >> 7
    else
bank    set -1
    endif
    endm

    ;; Only issue "banksel" if the bank differs from the last bank.
    ;; This must not be used across branching.
selbnkc     macro   file
    if bank != (file) >> 7
    selbank file
    endif
    endm

    ;; Only issue "banksel" if the bank differs from prevfile.
    ;; This is useful across branches, if you can guarantee the same bank for all origins.
selbnkd     macro   file, prevfile
    if      bank < 0 || (file) >> 7 != (prevfile) >> 7
    selbank file
    endif
    endm

    ;; Z := *file == 0
    ;;
    ;; Clobbers W.
zero16f macro   file
    selbank file
    movf    file, W
    selbnkc file + 1
    iorwf   file + 1, W
    endm

    ;; Signed 16-bit greater-than.
    ;;
    ;; C := (int) *file1 > (int) *file2
    ;;
    ;; Clobbers W.
gts16ff     macro   file1, file2
    selbank file2
    movf    file2, W
    selbnkc file1
    subwf   file1, W

    selbnkc file2 + 1
    movf    file2 + 1, W
    btfss   STATUS, C
    incf    file2 + 1, W
    selbnkc file1 + 1
    subwf   file1 + 1, W

    bsf     STATUS, C
    andlw   0x80
    btfss   STATUS, Z
    bcf     STATUS, C
    endm

    ;; Unsigned 16-bit greater-than.
    ;;
    ;; Z := *file == lit
    ;; C := *file > lit
    ;;
    ;; Clobbers acc1, W.
gt16fl  macro   file, lit
    selbank file + 1
    movf    file + 1, W
    sublw   HIGH (lit)
    selbnkc file
    movf    STATUS, W
    movwf   acc1
    movf    file, W
    btfsc   acc1, Z
    sublw   LOW (lit)
    movf    STATUS, W
    xorlw   (1 << C)
    movwf   STATUS
    endm

    ;; Unsigned 16-bit greater-than-or-equal.
    ;;
    ;; Z := *file == lit
    ;; C := *file >= lit
ge16lf  macro   file, lit
    movlw   HIGH (lit)
    selbank file + 1
    subwf   file + 1, W
    movlw   LOW (lit)
    selbnkc file
    btfsc   STATUS, Z
    subwf   file, W
    endm

    ;; if ((int) (*file1 - lit) >= 0) goto dest
bge16lf macro   file1, lit, dest
    local   mend

    movlw   HIGH dest
    movwf   PCLATH
    selbank file1 + 1
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
    selbnkc file1
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
    selbank file1 + 1
    movf    file1 + 1, W
    selbnkc file2 + 1
    subwf   file2 + 1, W
    btfss   STATUS, C
    goto    dest

    movlw   HIGH mend
    movwf   PCLATH
    btfss   STATUS, Z
    goto    mend

    movlw   HIGH dest
    movwf   PCLATH
    selbnkc file2
    movf    file2, W
    selbnkc file1
    subwf   file1, W
    btfsc   STATUS, C
    goto    dest
mend:
    endm

    ;; Performs a left-rotate n times on file, setting low bits to carry.
    ;;
    ;; C is the last rotated-out bit, just like for rlf.
rl16fl      macro   file, n, carry
    local   i

    selbank file + 1

i   set     0
    while   i < n
i   set     i + 1
    if carry
    bsf     STATUS, C
    else
    bcf     STATUS, C
    endif
    selbnkd file, file + 1
    rlf     file, F
    selbnkc file + 1
    rlf     file + 1, F
    endw
    endm

    ;; Performs a right-rotate n times on file, setting high bits to carry.
    ;;
    ;; C is the last rotated-out bit, just like for rrf.
rr16fl      macro   file, n, carry
    local   i

    selbank file

i   set     0
    while   i < n
i   set     i + 1
    if carry
    bsf     STATUS, C
    else
    bcf     STATUS, C
    endif
    selbnkd file + 1, file
    rrf     file + 1, F
    selbnkc file
    rrf     file, F
    endw
    endm

clr16f  macro   file
    selbank file + 1
    clrf    file + 1
    selbnkc file
    clrf    file
    endm

    ;; dest += file
add16ff macro   file, dest
    selbank file + 1
    movf    file + 1, W
    selbnkc dest + 1
    addwf   dest + 1, F
    selbnkc file
    movf    file, W
    selbnkc dest
    addwf   dest, F
    selbnkc dest + 1
    btfsc   STATUS, C
    incf    dest + 1, F
    endm

    ;; dest16 += l
add16lf macro   l, dest
    selbank dest + 1
    if      HIGH (l) != 0
    movlw   HIGH (l)
    addwf   dest + 1, F
    endif
    movlw   LOW (l)
    selbnkc dest
    addwf   dest, F
    selbnkc dest + 1
    btfsc   STATUS, C
    incf    dest + 1, F
    endm

    ;; dest16 += file8
add0816ff macro   file8, dest16
    selbank file8
    movf    file8, W
    selbnkc dest16
    addwf   dest16, F
    selbnkc dest16 + 1
    btfsc   STATUS, C
    incf    dest16 + 1, F
    endm

    ;; dest += file
    ;;
    ;; Where the files are signed 16-bit values, and the operation is saturated at -MIN and +MAX.
    ;;
    ;; Low saturation happened if sign(dest) && sign(file) != sign(dest + file)
    ;; High saturation happened if !sign(dest) && !sign(file) != sign(dest + file)
    ;;
    ;; Clobbers acc1.
satadds16ff macro   file, dest
    local   mend

    selbank file + 1
    movf    file + 1, W
    andlw   0x80
    movwf   acc1
    selbnkc dest + 1
    movf    dest + 1, W
    andlw   0x80
    xorwf   acc1, W
    btfsc   STATUS, Z
    bsf     acc1, 0             ; Bit 0: whether signs are equal, so saturation may happen.
    movf    dest + 1, W
    andlw   0x80
    andwf   acc1, W
    btfsc   STATUS, Z
    bcf     acc1, 7             ; Bit 7: whether both files are negative.

    add16ff file, dest

    ;; Check if saturation was possible.
    ;; if (!acc1:0) goto mend
    movlw   HIGH mend
    movwf   PCLATH
    btfss   acc1, 0
    goto    mend

    ;; Check if saturation happened.
    ;; if (sign(dest) == acc1:7) goto mend
    selbnkc dest + 1
    movf    dest + 1, W
    xorwf   acc1, W
    andlw   0x80

    btfsc   STATUS, Z
    goto    mend

    ;; Saturation happened.

    selbnkc dest
    clrf    dest
    btfss   acc1, 7
    comf    dest, F

    selbnkc dest + 1
    movlw   0x80
    movwf   dest + 1
    btfss   acc1, 7
    comf    dest + 1, F

mend:
    endm

    ;; dest += C
    ;;
    ;; Useful for rounding after a rrf16 on fixed point values.
    ;; Awkward name because addcf is a valid instruction name.
    ;;
    ;; Clobbers W.
add08cf macro   dest
    selbank dest
    btfsc   STATUS, C
    incf    dest, F
    endm

add16cf macro   dest
    local   mend

    movlw   HIGH mend
    movwf   PCLATH
    selbank dest
    btfss   STATUS, C
    goto    mend

    movlw   1
    addwf   dest, F
    selbnkc dest + 1
    btfsc   STATUS, C
    incf    dest + 1, F
mend:
    endm

    ;; dest -= file
sub16ff macro   file, dest
    selbank file + 1
    movf    file + 1, W
    selbnkc dest + 1
    subwf   dest + 1, F
    selbnkc file
    movf    file, W
    selbnkc dest
    subwf   dest, F
    selbnkc dest + 1
    btfss   STATUS, C
    decf    dest + 1, F
    endm

    ;; dest16 -= file8
sub0816ff macro   file8, dest16
    selbank file8
    movf    file8, W
    selbnkc dest16
    subwf   dest16, F
    selbnkc dest16 + 1
    btfss   STATUS, C
    decf    dest16 + 1, F
    endm

    ;; file *= lit
    ;;
    ;; Clobbers acc1, acc2.
mul16fl macro   file, lit
    local   i, j

    if      file == acc1
    error   "Cannot use w16 in mul16fl"
    endif

    if      lit != 0
    clrf    acc1
    clrf    acc2

    selbank file + 1

i   set     0
j   set     lit
    if      lit < 0
j   set     -j
    endif
    while   i < 16 && j > 0
i   set i + 1

    if j & 1
    selbnkd file + 1, file + 1
    movf    file + 1, W
    addwf   acc2, F
    selbnkc file
    movf    file, W
    addwf   acc1, F
    btfsc   STATUS, C
    incf    acc2, F
    endif

    bcf     STATUS, C
    selbnkc file
    rlf     file, F
    selbnkc file + 1
    rlf     file + 1, F

j   set     j >> 1
    endw
    if      lit < 0
    neg16f  acc1
    endif
    mov16ff acc1, file
    else                        ; lit == 0
    clr16f  file
    endif
    endm

    ;; file *= W
mul16wf macro   file
    local   mulloop
    local   mulskip

    clrf    acc1
    clrf    acc2

    movwf   acc3

    selbank file

mulloop:
    bcf     STATUS, C
    rrf     acc3, F

    movlw   HIGH mulskip
    movwf   PCLATH
    btfss   STATUS, C
    goto    mulskip

    selbnkd file + 1, file
    movf    file + 1, W
    addwf   acc2, F
    selbnkc file
    movf    file, W
    addwf   acc1, F
    btfsc   STATUS, C
    incf    acc2, F
mulskip:

    bcf     STATUS, C
    rlf     file, F
    selbnkc file + 1
    rlf     file + 1, F

    movlw   HIGH mulloop
    movwf   PCLATH
    movf    acc3, F
    btfss   STATUS, Z
    goto    mulloop

    mov16ff acc1, file
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
    selbank file
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

    ;; file = -file
    ;;
    ;; Clobbers STATUS.
neg16f  macro   file
    selbank file
    comf    file, F
    incf    file, F
    selbnkc file + 1
    btfsc   STATUS, Z
    decf    file + 1, F
    comf    file + 1, F
    endm

mov16ff macro   src, dest
    selbank src + 1
    movf    src + 1, W
    selbnkc dest + 1
    movwf   dest + 1
    selbnkc src
    movf    src, W
    selbnkc dest
    movwf   dest
    endm

mov16lf macro   l, file
    movlw   LOW (l)
    selbank file
    movwf   file
    selbnkc file + 1
    movlw   HIGH (l)
    movwf   file + 1
    endm

mov16lw macro   l
    mov16lf l, w16
    endm

mov0816ff   macro   src, dest
    selbank src
    movf    src, W
    selbnkc dest
    movwf   dest
    selbnkc dest + 1
    clrf    dest + 1
    endm

    ;; Extend signed 8-bit src to 16-bit dest.
movs0816ff   macro   src8, dest16
    selbank src8
    movf    src8, W
    selbnkc dest16
    movwf   dest16
    bcf     STATUS, DC
    btfsc   dest16, 7
    bsf     STATUS, DC
    selbnkc dest16 + 1
    clrf    dest16 + 1
    btfsc   STATUS, DC
    comf    dest16 + 1, F
    endm

    ;; W = file16 % l
    ;; file16 /= l
divmod1608fl    macro   file16, l
    local   divloop
    local   divend
    if      file16 == acc1
    error   "Cannot use acc1 for divmod16"
    endif
    selbank file16
    movf    file16, W
    movwf   acc1
    clrf    file16
    selbnkc file16 + 1
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

    selbnkd file16, file16 + 1
    incf    file16, F
    selbnkc file16 + 1
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

    selbank file
i   set     0
    while   i < l
i   set     i + 1
    bcf     STATUS, C
    rrf     file, F
    endw
    endm

    ;; Divide signed 16-bit file by 2^l.
divp2s16lf macro   file, l
    local   i

    selbank file + 1
    movlw   0x80
    andwf   file + 1, W
    bcf     STATUS, DC
    btfss   STATUS, Z
    bsf     STATUS, DC

    selbnkc file

i   set     0
    while   i < l
i   set     i + 1
    bcf     STATUS, C
    btfsc   STATUS, DC
    bsf     STATUS, C
    selbnkd file + 1, file
    rrf     file + 1, F
    selbnkc file
    rrf     file, F
    endw
    endm
    endif
