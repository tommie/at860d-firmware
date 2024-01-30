    ;; Temperature Controller

    ifdef module_tempcontrol
#undefine module_tempcontrol
    endif
#define module_tempcontrol

    ifndef module_heater
    error "heater module is a dependency of tempcontrol"
    endif
    ifndef module_temp
    error "temp module is a dependency of tempcontrol"
    endif

    ifdef section_udata
TEMPC_P         equ 1           ; 4.4 fixed point.
TEMPC_I         equ 0           ; 4.4 fixed point.
TEMPC_D         equ 0           ; 4.4 fixed point.

tempc_value     res 1           ; Unit of 2 *C.
tempc_err       res 2           ; 12.4 fixed point.
tempc_prev_err  res 2           ; 12.4 fixed point.
tempc_quad      res 2           ; 12.4 fixed point.
tempc_acc       res 2           ; 12.4 fixed point.
    endif

    ifdef section_code
tempc_idle  macro
    ;; tempc_err = tempc_value - temp_value
    banksel tempc_value
    movf    tempc_value, W
    banksel tempc_err
    movwf   tempc_err
    banksel tempc_err + 1
    clrf    tempc_err + 1
    rl16fl  tempc_err, 5, 0
    sub16ff tempc_err, temp_value

    ;; tempc_quad += tempc_err (assuming constant time between tempc_idle calls)
    add16ff tempc_err, tempc_quad

    ;; tempc_acc = D * delta(tempc_err) (assuming constant time between tempc_idle calls)
    mov16ff tempc_prev_err, tempc_acc
    sub16ff tempc_err, tempc_acc

    mul16fl tempc_acc, TEMPC_D
    mov16ff w16, tempc_acc
    rr16fl  tempc_acc, 4, 0

    ;; tempc_acc += P * tempc_err + I * int(tempc_err)
    mul16fl tempc_err, TEMPC_P
    add16ff w16, tempc_acc
    rr16fl  tempc_acc, 4, 0

    mul16fl tempc_quad, TEMPC_I
    add16ff w16, tempc_acc
    ;; tempc_acc is now 8.8 fixed point.

    ;; heater_setw(tempc_acc < 0 ? 0 : (tempc_acc >> 8))
    banksel tempc_acc + 1
    movf    tempc_acc + 1, W
    btfsc   tempc_acc + 1, 7
    clrw
    heater_setw
    endm

    ;; Sets the set-temperature as temp = W * 2 [*C].
tempc_setw  macro
    banksel tempc_value
    movwf   tempc_value
    endm

    ;; Gets the set-temperature as W = temp / 2 [*C].
tempc_getw  macro
    banksel tempc_value
    movf    tempc_value, W
    endm

    ;; Gets the set-temperature as *C in 8.0 fixed point.
tempc_get16 macro   dest
    banksel tempc_value
    movf    tempc_value, W
    banksel dest
    movwf   dest
    banksel dest + 1
    clrf    dest + 1
    rl16fl  dest, 1, 0
    endm
    endif

    ifdef section_init
    clr16f  tempc_value
    clr16f  tempc_prev_err
    clr16f  tempc_quad
    endif

    ifdef section_idle
    tempc_idle
    endif
