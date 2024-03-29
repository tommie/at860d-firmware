    ;; PID Controller
    ;;
    ;; Based on the hardware implementation of the PIC16F161X:
    ;;
    ;;   https://ww1.microchip.com/downloads/en/DeviceDoc/PIC16(L)F16148-14-20-Pin-8-Bit-Flash-MCU-Data-sheet.pdf
    ;;
    ;; TB3136
    ;;   PID Control on PIC16F161X by using a PID Peripheral
    ;;   https://ww1.microchip.com/downloads/en/Appnotes/90003136A.pdf
    ;;
    ;; AN964 (an older AN)
    ;;   Software PID Control of an Inverted Pendulum Using the PIC16F684
    ;;   https://ww1.microchip.com/downloads/en/AppNotes/00964A.pdf

    ifdef module_pid
#undefine module_pid
    endif
#define module_pid

    ifdef section_udata
    ;; Reserves udata space for one PID controller using compile-time parameters.
    ;;
    ;; The index must be a unique integer, but is otherwise uninterpreted.
    ;; The `n` defines the number of bytes used to store the input and output values.
    ;; The `round` tells how many bits to round away after each multiplication.
    ;; `max_err` is the allowed range [-max_err, max_err] before clamping.
pid_lit_udata   macro   index, n, kp, ki, kd, t, round, max_err, max_acc
pid_out#v(index)        res n
pid_z0_#v(index)        res n
pid_z1_#v(index)        res n
pid_z2_#v(index)        res n

pid_n#v(index)          equ n
pid_k1_#v(index)        equ kp + ki*t + kd/t
pid_k2_#v(index)        equ -(kp + 2*kd/t)
pid_k3_#v(index)        equ kd/t
pid_rd#v(index)         equ round
pid_max_err#v(index)    equ max_err
pid_max_acc#v(index)    equ max_acc
    endm
    endif

    ifdef section_code
pid_lit_reset   macro   index
    clr16f  pid_out#v(index)
    clr16f  pid_z0_#v(index)
    clr16f  pid_z1_#v(index)
    clr16f  pid_z2_#v(index)
    endm

    ;; Initializes the data of a `pid_lit_udata`.
pid_lit_init    macro   index
    if              pid_n#v(index) == 2
    pid_lit_reset   index
    else
    error           "pid_lit is not implemented for this word size"
    endif
    endm

    ;; Runs an update with new samples of set-value and measured value.
    ;;
    ;; The `index` refers to a `pid_lit_udata` index.
pid_lit_update  macro   index, setfile, realfile
    if      pid_n#v(index) == 2
    ;; z2 = z1
    mov16ff     pid_z1_#v(index), pid_z2_#v(index)

    ;; z1 = z0
    mov16ff     pid_z0_#v(index), pid_z1_#v(index)

    ;; z0 = *setfile - *realfile
    mov16ff     setfile, pid_z0_#v(index)
    sub16ff     realfile, pid_z0_#v(index)
    clamps16fl  pid_z0_#v(index), pid_max_err#v(index)

    ;; out += k3 * z2
    mov16ff     pid_z2_#v(index), w16
    sfpmul16lf  pid_k3_#v(index), w16, pid_rd#v(index)
    satadds16ff w16, pid_out#v(index)

    ;; out += k2 * z1
    mov16ff     pid_z1_#v(index), w16
    sfpmul16lf  pid_k2_#v(index), w16, pid_rd#v(index)
    satadds16ff w16, pid_out#v(index)

    ;; out += k1 * z0
    mov16ff     pid_z0_#v(index), w16
    sfpmul16lf  pid_k1_#v(index), w16, pid_rd#v(index)
    satadds16ff w16, pid_out#v(index)

    clamps16fl  pid_out#v(index), pid_max_acc#v(index)
    endif
    endm

pid_get16   macro   index, dest
    if      pid_n#v(index) == 2
    mov16ff pid_out#v(index), dest
    endif
    endm
    endif
