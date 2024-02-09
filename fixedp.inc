    ;; Fixed Point Arithmetic for PIC14.

    ifdef   section_code

    ;; dest *= lit
    ;;
    ;; Allows unsigned 16x16 multiplication in `16-frac+1` bits with
    ;; configurable precision. It rounds the output *by* (removes)
    ;; `frac` bits.
    ;;
    ;; If dest is FPa.b and lit is FPc.d, the output
    ;; is an FP(a + c).(d - frac).
ufpmul16lf  macro   lit, dest, frac
    fpmul16lf_      lit, dest, frac, 0
    endm

sfpmul16lf  macro   lit, dest, frac
    fpmul16lf_      lit, dest, frac, 1
    endm

fpmul16lf_  macro   lit, dest, frac, signed
    local       l
    local       i
    local       j
    local       i25

    if          dest == acc1 || dest == acc2 || dest == acc3
    error       "Cannot use acc1-3 in *fpmul16lf"
    endif
    clr16f      acc1

    if          frac > 16
    error       "Assertion failed: frac <= 16"
    endif

    ;; First step: multiply with the first frac bits of src, but keep reducing precision.
l   set         lit
    if          lit < 0
l   set         -(lit)
    endif
i   set         0
    while       i < frac && l != 0
i   set         i + 1
    if          signed
    divp2s16lf  acc1, 1
    else
    rr16fl      acc1, 1, 0
    endif

    if          l & 1
    add16ff     dest, acc1
    endif

l   set         l >> 1
    endw

    ;; Second step: multiply with the last bits of src, as a normal integer.
j   set         i
    while       j < 16 && l != 0
j   set         j + 1
    rl16fl      dest, 1, 0

    if          l & 1
    add16ff     dest, acc1
    endif

l   set         l >> 1
    endw

    ;; Third step: round away the last bit. Because we stopped the
    ;; first step when l==0, we may need to remove more bits.
i25 set         i
    if          signed
    divp2s16lf  acc1, 1 + (frac) - i25
    else
    rr16fl      acc1, 1 + (frac) - i25, 0
    endif
    add16cf     acc1

    ;; Fourth step; handle negative lit.
    if          lit < 0
    neg16f      acc1
    endif

    mov16ff     acc1, dest
    endm

    ;; dest *= W
ufpmul0816wf    macro   dest, frac
    local       i

    if          dest == acc1 || dest == acc2 || dest == acc3
    error       "Cannot use acc1-3 in ufpmul160816wf"
    endif
    clr16f      acc1

    movwf       acc3

    if          frac > 8
    error       "Assertion failed: frac <= 8"
    endif

    ;; First step: multiply with the first frac bits of src, but keep reducing precision.
i   set                 0
    while               i < frac
i   set                 i + 1
    ufpmul0816wf_add_   dest, 1
    endw

    ;; Second step: multiply with the last bits of src, as a normal integer.
    while               i < 8
i   set                 i + 1
    ufpmul0816wf_add_   dest, 0
    endw

    ;; Third step: round away the last bit.
    rr16fl      acc1, 1, 0
    add16cf     acc1

    mov16ff     acc1, dest
    endm

    ;; Internal macro.
ufpmul0816wf_add_   macro   dest, isfrac
    local       add_done

    if          isfrac
    rr16fl      acc1, 1, 0
    else
    rl16fl      dest, 1, 0
    endif

    pagesel     add_done
    btfss       acc3, 0
    goto        add_done

    add16ff     dest, acc1
add_done:

    bcf         STATUS, C
    rrf         acc3, F
    endm

    endif   ; section_code
