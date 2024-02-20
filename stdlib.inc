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
    ;; Issues a banksel, and updates the bank variable.
    ;; Ignores bank-agnostic files.
selbank macro   file
    ;; Files in udata won't be allocated here, and their addresses default to
    ;; zero, causing this to be safe, but only if we don't include INDF.
    if          ((file) & 0x7F) < 0x70 && (file) != 0x02 && (file) != 0x03 && (file) != 0x0A && (file) != 0x0B
    banksel     file
    endif
    endm

table_jump  macro   tblf
    local   case0

    selbank tblf
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

    ;; Z := *file == 0
    ;;
    ;; Clobbers W.
zero16f macro   file
    selbank file
    movf    file, W
    selbank file + 1
    iorwf   file + 1, W
    endm

    ;; Copies a bit from one file to another.
    ;;
    ;; The source and destination registers are only accessed once each,
    ;; making it safe to run on ports with interrupts enabled.
    ;;
    ;; Clobbers W, STATUS.
movbit  macro   srcf, srcb, destf, destb
    selbank srcf
    movwf   srcf, W
    andlw   1 << (srcb)
    selbank destf
    btfsc   STATUS, Z
    bcf     destf, destb
    btfss   STATUS, Z
    bsf     destf, destb
    endm

    ;; Like movbit, but inverts the result.
movinvbit   macro   srcf, srcb, destf, destb
    selbank srcf
    movf    srcf, W
    andlw   1 << (srcb)
    selbank destf
    btfsc   STATUS, Z
    bsf     destf, destb
    btfss   STATUS, Z
    bcf     destf, destb
    endm

    ;; Z := file1 == file2
    ;;
    ;; Clobbers W, STATUS.
eq16ff  macro   file1, file2
    bsf     STATUS, DC
    selbank file2
    movf    file2, W
    selbank file1
    xorwf   file1, W
    btfss   STATUS, Z
    bcf     STATUS, DC

    selbank file2 + 1
    movf    file2 + 1, W
    selbank file1 + 1
    xorwf   file1 + 1, W
    btfss   STATUS, Z
    bcf     STATUS, DC

    bcf     STATUS, Z
    btfsc   STATUS, DC
    bsf     STATUS, Z
    endm

    ;; Signed 16-bit greater-than.
    ;;
    ;; C := (int) *file1 > (int) *file2
    ;;
    ;; Clobbers W.
gts16ff     macro   file1, file2
    selbank file2
    movf    file2, W
    selbank file1
    subwf   file1, W

    selbank file2 + 1
    movf    file2 + 1, W
    btfss   STATUS, C
    incf    file2 + 1, W
    selbank file1 + 1
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
    movf    STATUS, W
    movwf   acc1
    selbank file
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
    selbank file
    btfsc   STATUS, Z
    subwf   file, W
    endm

    ;; if ((int) (*file1 - lit) >= 0) goto dest
bge16lf macro   file1, lit, dest
    local   mend

    pagesel dest
    selbank file1 + 1
    movf    file1 + 1, W
    sublw   HIGH lit
    btfss   STATUS, C
    goto    dest

    pagesel mend
    btfss   STATUS, Z
    goto    mend

    pagesel dest
    movlw   LOW lit
    selbank file1
    subwf   file1, W
    btfsc   STATUS, C
    goto    dest
mend:
    endm

    ;; if ((int) (*file1 - *file2) >= 0) goto dest
bge16ff macro   file1, file2, dest
    local   mend

    pagesel dest
    selbank file1 + 1
    movf    file1 + 1, W
    selbank file2 + 1
    subwf   file2 + 1, W
    btfss   STATUS, C
    goto    dest

    pagesel mend
    btfss   STATUS, Z
    goto    mend

    pagesel dest
    selbank file2
    movf    file2, W
    selbank file1
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

    selbank file

i   set     0
    while   i < n
i   set     i + 1
    if      carry
    bsf     STATUS, C
    else
    bcf     STATUS, C
    endif
    selbank file
    rlf     file, F
    selbank file + 1
    rlf     file + 1, F
    endw
    endm

    ;; Performs a right-rotate n times on file, setting high bits to carry.
    ;;
    ;; C is the last rotated-out bit, just like for rrf.
rr16fl      macro   file, n, carry
    local   i

    selbank file + 1

i   set     0
    while   i < n
i   set     i + 1
    if      carry
    bsf     STATUS, C
    else
    bcf     STATUS, C
    endif
    selbank file + 1
    rrf     file + 1, F
    selbank file
    rrf     file, F
    endw
    endm

clr16f  macro   file
    selbank file + 1
    clrf    file + 1
    selbank file
    clrf    file
    endm

    ;; dest += file
add16ff macro   file, dest
    selbank file + 1
    movf    file + 1, W
    selbank dest + 1
    addwf   dest + 1, F
    selbank file
    movf    file, W
    selbank dest
    addwf   dest, F
    selbank dest + 1
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
    selbank dest
    addwf   dest, F
    selbank dest + 1
    btfsc   STATUS, C
    incf    dest + 1, F
    endm

    ;; dest16 += file8
add0816ff macro   file8, dest16
    selbank file8
    movf    file8, W
    selbank dest16
    addwf   dest16, F
    selbank dest16 + 1
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
    selbank dest + 1
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
    pagesel mend
    btfss   acc1, 0
    goto    mend

    ;; Check if saturation happened.
    ;; if (sign(dest) == acc1:7) goto mend
    selbank dest + 1
    movf    dest + 1, W
    xorwf   acc1, W
    andlw   0x80

    btfsc   STATUS, Z
    goto    mend

    ;; Saturation happened.

    selbank dest
    clrf    dest
    btfss   acc1, 7
    comf    dest, F

    selbank dest + 1
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

    pagesel mend
    btfss   STATUS, C
    goto    mend

    movlw   1
    selbank dest
    addwf   dest, F
    selbank dest + 1
    btfsc   STATUS, C
    incf    dest + 1, F
mend:
    endm

    ;; dest -= file
sub16ff macro   file, dest
    selbank file + 1
    movf    file + 1, W
    selbank dest + 1
    subwf   dest + 1, F
    selbank file
    movf    file, W
    selbank dest
    subwf   dest, F
    selbank dest + 1
    btfss   STATUS, C
    decf    dest + 1, F
    endm

    ;; dest16 -= file8
sub0816ff macro   file8, dest16
    selbank file8
    movf    file8, W
    selbank dest16
    subwf   dest16, F
    selbank dest16 + 1
    btfss   STATUS, C
    decf    dest16 + 1, F
    endm

    ;; file *= lit
    ;;
    ;; Clobbers acc1, acc2.
mul16fl macro   file, lit
    local   i, j

    if      file == acc1
    error   "Cannot use acc1 in mul16fl"
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
    selbank file + 1
    movf    file + 1, W
    addwf   acc2, F
    selbank file
    movf    file, W
    addwf   acc1, F
    btfsc   STATUS, C
    incf    acc2, F
    endif

    bcf     STATUS, C
    selbank file
    rlf     file, F
    selbank file + 1
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

    selbank file + 1

mulloop:
    bcf     STATUS, C
    rrf     acc3, F

    pagesel mulskip
    btfss   STATUS, C
    goto    mulskip

    selbank file + 1
    movf    file + 1, W
    addwf   acc2, F
    selbank file
    movf    file, W
    addwf   acc1, F
    btfsc   STATUS, C
    incf    acc2, F
mulskip:

    bcf     STATUS, C
    selbank file
    rlf     file, F
    selbank file + 1
    rlf     file + 1, F

    pagesel mulloop
    movf    acc3, F
    btfss   STATUS, Z
    goto    mulloop

    mov16ff acc1, file
    endm

    ;; file = -file
    ;;
    ;; Clobbers STATUS.
neg16f  macro   file
    selbank file
    comf    file, F
    incf    file, F
    selbank file + 1
    btfsc   STATUS, Z
    decf    file + 1, F
    comf    file + 1, F
    endm

mov16ff macro   src, dest
    selbank src + 1
    movf    src + 1, W
    selbank dest + 1
    movwf   dest + 1
    selbank src
    movf    src, W
    selbank dest
    movwf   dest
    endm

mov16lf macro   l, file
    movlw   LOW (l)
    selbank file
    movwf   file
    selbank file + 1
    movlw   HIGH (l)
    movwf   file + 1
    endm

mov16lw macro   l
    mov16lf l, w16
    endm

mov0816ff   macro   src, dest
    selbank src
    movf    src, W
    selbank dest
    movwf   dest
    selbank dest + 1
    clrf    dest + 1
    endm

    ;; Extend signed 8-bit src to 16-bit dest.
movs0816ff   macro   src8, dest16
    selbank src8
    movf    src8, W
    selbank dest16
    movwf   dest16
    bcf     STATUS, DC
    btfsc   dest16, 7
    bsf     STATUS, DC
    selbank dest16 + 1
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
    selbank file16 + 1
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
    pagesel divend
    btfss   STATUS, C
    goto    divend

    selbank file16
    incf    file16, F
    selbank file16 + 1
    btfsc   STATUS, Z
    incf    file16 + 1, F
    pagesel divloop
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

i   set     0
    while   i < l
i   set     i + 1
    bcf     STATUS, C
    btfsc   STATUS, DC
    bsf     STATUS, C
    selbank file + 1
    rrf     file + 1, F
    selbank file
    rrf     file, F
    endw
    if      l == 0
    bcf     STATUS, C
    endif
    endm

    ;; Count the number of leading zeros in W.
    ;;
    ;; Returns the count in W.
    ;;
    ;; Clobbers STATUS, acc1.
lzw macro
    local   mloop

    movwf   acc1
    pagesel mloop
    clrw

mloop:
    addlw   1
    bsf     STATUS, C
    rlf     acc1, F
    btfss   STATUS, C
    goto    mloop
    addlw   -1
    endm

    ;; Count the number of leading zeros in w16.
    ;;
    ;; Returns the count in W.
    ;;
    ;; Clobbers STATUS, acc1, w16.
lzw16   macro
    movf    w16 + 1, W
    lzw
    movwf   w16 + 1
    movlw   8
    subwf   w16 + 1, W
    btfss   STATUS, Z
    movf    w16, W
    lzw
    addwf   w16 + 1, W
    endm

    ;; Clamp signed *file16 within [0, lit_max].
clamps16f0l macro   file16, lit_max
    local   is_neg
    local   mend

    pagesel is_neg
    selbank file16 + 1
    btfsc   file16 + 1, 7
    goto    is_neg

    pagesel mend
    gt16fl  file16, lit_max
    btfss   STATUS, C
    goto    mend

    mov16lf lit_max, file16
    goto    mend

is_neg:
    clr16f  file16

mend:
    endm

    ;; Clamp signed *file16 within [-lit_abs_max, lit_abs_max].
    ;;
    ;; Clobbers w16, W, STATUS, PCLATH.
clamps16fl  macro   file16, lit_abs_max
    local   is_neg
    local   mend

    pagesel is_neg
    selbank file16 + 1
    btfsc   file16 + 1, 7
    goto    is_neg

    pagesel mend
    ge16lf  file16, lit_abs_max
    btfss   STATUS, C
    goto    mend

    mov16lf lit_abs_max, file16
    goto    mend

is_neg:
    mov16ff file16, w16
    neg16f  w16

    pagesel mend
    ge16lf  w16, lit_abs_max
    btfss   STATUS, C
    goto    mend

    mov16lf -(lit_abs_max), file16

mend:
    endm
    endif   ; section_code
