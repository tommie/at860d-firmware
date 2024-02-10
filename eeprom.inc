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
    movlw   LOW (addr)
    eeprom_read_w
    endm

eeprom_read_w   macro
    selbank EEADR
    movwf   EEADR
    eeprom_read
    endm

eeprom_read16_lf    macro addr, dest
    eeprom_read_lw  (addr)
    selbank dest
    movwf   dest
    eeprom_read_lw  (addr) + 1
    selbnkc (dest) + 1
    movwf   (dest) + 1
    endm

eeprom_read macro
    selbank EECON1
    bcf     EECON1, EEPGD
    bsf     EECON1, RD
    selbnkc EEDAT
    movf    EEDAT, W
    endm

eeprom_write_lw     macro addr
    if      addr < 0x2100 || addr >= 0x2200
    error   "EEPROM address out of range"
    endif
    selbank EEDAT
    movwf   EEDAT
    movlw   addr
    selbnkc EEADR
    movwf   EEADR
    eeprom_write
    endm

eeprom_write_w      macro
    selbank EEDAT
    movwf   EEDAT
    eeprom_write
    endm

eeprom_write    macro
    local   wait_gie

    selbank EECON1
    bcf     EECON1, EEPGD
    bsf     EECON1, WREN
    bcf     INTCON, GIE
    movlw   HIGH wait_gie
    movwf   PCLATH

wait_gie:
    btfsc   INTCON, GIE
    goto    wait_gie

    movlw   0x55
    selbnkc EECON2
    movwf   EECON2
    movlw   0xAA
    movwf   EECON2
    bsf     EECON1, WR
    bsf     INTCON, GIE
    endm

eeprom_write_lw16   macro addr
    movf    w16 + 1, W
    selbank ee_dath
    movwf   ee_dath
    selbnkc ee_state
    bsf     ee_state, EE_WR_W16
    movf    w16, W
    eeprom_write_lw addr
    endm

eeprom_skip_if_can_write    macro
    bcf     STATUS, Z
    movlw   -2
    selbank EECON1
    btfss   EECON1, WR
    addlw   1
    selbnkc PIR2
    btfss   PIR2, EEIF
    addlw   1
    btfss   STATUS, Z
    endm

eeprom_idle macro
    local   done

    movlw   HIGH done
    movwf   PCLATH
    selbank EECON1
    btfsc   EECON1, WR
    goto    done

    selbnkc PIR2
    btfss   PIR2, EEIF
    goto    done

    bcf     PIR2, EEIF
    selbnkc EECON1
    bcf     EECON1, WREN

    selbnkc ee_state
    btfss   ee_state, EE_WR_W16
    goto    done

    bcf     ee_state, EE_WR_W16
    selbnkc EEADR
    incf    EEADR, 1
    selbnkc ee_dath
    movf    ee_dath, W
    eeprom_write_w

done:
    endm
    endif

    ifdef section_init
    selbank ee_state
    clrf    ee_state
    endif

    ifdef section_idle
    eeprom_idle
    endif
