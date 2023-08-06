    ;; Non-volatile data storage.

    ifdef module_eeprom
#undefine module_eeprom
    endif
#define module_eeprom

    ifdef section_udata
ee_dath     res 1
ee_state    res 1
    endif

EE_WR_W16   equ 0

    ifdef section_code
eeprom_read_lw      macro addr
    if      addr < 0x2100 || addr >= 0x2200
    error   "EEPROM address out of range"
    endif
    movlw   LOW addr
    banksel EEADR
    movwf   EEADR
    banksel EECON1
    bcf     EECON1, EEPGD
    bsf     EECON1, RD
    banksel EEDAT
    movf    EEDAT, W
    endm

eeprom_read_lw16    macro addr, dest
    eeprom_read_lw  addr
    banksel dest
    movlw   dest
    eeprom_read_lw  addr + 1
    banksel dest + 1
    movlw   dest + 1
    endm

eeprom_write_lw     macro addr
    if      addr < 0x2100 || addr >= 0x2200
    error   "EEPROM address out of range"
    endif
    banksel EEDAT
    movwf   EEDAT
    movlw   addr
    banksel EEADR
    movwf   EEADR
    eeprom_write
    endm

eeprom_write_w      macro
    banksel EEDAT
    movwf   EEDAT
    eeprom_write
    endm

eeprom_write    macro
    local   wait_gie

    banksel EECON1
    bcf     EECON1, EEPGD
    bsf     EECON1, WREN
    bcf     INTCON, GIE
    movlw   HIGH wait_gie
    movwf   PCLATH

wait_gie:
    btfsc   INTCON, GIE
    goto    wait_gie

    movlw   0x55
    movwf   EECON2
    movlw   0xAA
    movwf   EECON2
    bsf     EECON1, WR
    bsf     INTCON, GIE
    endm

eeprom_write_lw16   macro addr
    movf    w16 + 1, W
    banksel ee_dath
    movwf   ee_dath
    banksel ee_state
    bsf     ee_state, EE_WR_W16
    movf    w16, W
    eeprom_write_lw addr
    endm

eeprom_skip_if_can_write    macro
    bcf     STATUS, Z
    movlw   -2
    banksel EECON1
    btfss   EECON1, WR
    addlw   1
    banksel PIR2
    btfss   PIR2, EEIF
    addlw   1
    btfss   STATUS, Z
    endm

eeprom_idle macro
    local   done

    movlw   HIGH done
    movwf   PCLATH
    banksel EECON1
    btfsc   EECON1, WR
    goto    done

    banksel PIR2
    btfss   PIR2, EEIF
    goto    done

    bcf     PIR2, EEIF
    bcf     EECON1, WREN

    banksel ee_state
    btfss   ee_state, EE_WR_W16
    goto    done

    bcf     ee_state, EE_WR_W16
    banksel EEADR
    incf    EEADR, 1
    banksel ee_dath
    movf    ee_dath, W
    eeprom_write_w

done:
    endm
    endif

    ifdef section_init
    banksel ee_state
    clrf    ee_state
    endif

    ifdef section_idle
    eeprom_idle
    endif
