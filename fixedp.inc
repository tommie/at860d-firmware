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
    _fpmul16lf      lit, dest, frac, 0
    endm

sfpmul16lf  macro   lit, dest, frac
    _fpmul16lf      lit, dest, frac, 1
    endm

_fpmul16lf  macro   lit, dest, frac, signed
    if          file == acc1 || file == acc2 || file == acc3
    error       "Cannot use acc1-3 in mul16fl"
    endif
    clr16f      acc1

    if          frac > 16
    error       "Assertion failed: frac <= 16"
    endif

    ;; First step: multiply with the first frac bits of src, but keep reducing precision.
i   set         0
l   set         lit
    if          lit < 0
l   set         -(lit)
    endif
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
    while       i < 16 && l != 0
i   set         i + 1
    rl16fl      dest, 1, 0

    if          l & 1
    add16ff     dest, acc1
    endif
l   set         l >> 1
    endw

    ;; Third step: round away the last bit.
    if          signed
    divp2s16lf  acc1, 1
    else
    rr16fl      acc1, 1, 0
    endif
    add16cf     acc1

    ;; Fourth step; handle negative lit.
    if          lit < 0
    neg16f      acc1
    endif

    mov16ff     acc1, dest
    endm

    endif   ; section_code
