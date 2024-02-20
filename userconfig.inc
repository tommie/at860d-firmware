    ;; Persistent user configuration in data EEPROM.

    ifdef module_userconfig
#undefine module_userconfig
    endif
#define module_userconfig

    ifndef module_airpump
    error "airpump module is a dependency of userconfig"
    endif
    ifndef module_eeprom
    error "eeprom module is a dependency of userconfig"
    endif
    ifndef module_fault
    error "fault module is a dependency of userconfig"
    endif
    ifndef module_tempcontrol
    error "tempcontrol module is a dependency of userconfig"
    endif

    ifdef section_udata
uc_state        res 1
uc_index        res 1

UC_EE_WRITE     equ 1
    endif

    ifdef section_eedata
    ;; Defines EEPROM data for a temperature [*C].
uc_dtemp    macro   value
    de      LOW ((value) << 1), HIGH ((value) << 1) ; FP9.1
    endm

uc_ee_temp
    ifndef      TEMPC_DIRECT_CONTROL
    uc_dtemp    TEMPCONTROL_MIN_TEMP >> 1
    uc_dtemp    50
    uc_dtemp    200
    uc_dtemp    60
    else
    uc_dtemp    0
    uc_dtemp    0
    uc_dtemp    0
    uc_dtemp    0
    endif       ; TEMPC_DIRECT_CONTROL
uc_ee_air   de  50, (AIRPUMP_MAX_RATIO) / 2, AIRPUMP_MAX_RATIO, AIRPUMP_MAX_RATIO
    endif

    ifdef section_code
    ;; Loads configuration from slot `index`.
uc_load macro           index
    eeprom_read16_lf    uc_ee_temp + 2*(index), w16
    tempc_set16         w16

    eeprom_read_lw      uc_ee_air + index
    airpump_setw
    endm

    ;; Sets the slot that will be used by `uc_store`.
uc_store    macro   index
    movlw   index
    selbank uc_index
    movwf   uc_index
    selbank uc_state
    bsf     uc_state, UC_EE_WRITE
    endm

uc_skip_if_can_store    macro
    selbank uc_state
    btfsc   uc_state, UC_EE_WRITE
    endm

uc_idle macro
    local   writing_done
    local   done

    ;; if (!(uc_state & (1 << UC_EE_WRITE))) goto write_ee_done
    pagesel done
    selbank uc_state
    btfss   uc_state, UC_EE_WRITE
    goto    done

    ifndef  DISABLE_FAULT
    ;; if (!fault_is_good) goto writing_done
    pagesel writing_done
    fault_is_good
    btfss   STATUS, Z
    goto    writing_done
    endif   ; DISABLE_FAULT

    uc_try_write_ee16   tempc_value, uc_ee_temp, 2
    uc_try_write_ee     airpump_value, uc_ee_air, 1

    pagesel done
    eeprom_skip_if_can_write
    goto    done

writing_done:
    ;; Writing all values is done.
    selbank uc_state
    bcf     uc_state, UC_EE_WRITE
done:
    endm

    ;; Starts writing file at eeaddr+uc_index*stride, if there is no
    ;; write in progress.
uc_try_write_ee macro   file, eeaddr, stride
    eeprom_try_write_fl file, eeaddr, uc_index, stride
    endm

uc_try_write_ee16   macro   file16, eeaddr, stride
    uc_try_write_ee file16, eeaddr, stride
    uc_try_write_ee file16 + 1, eeaddr + 1, stride
    endm
    endif

    ifdef section_init
    selbank uc_state
    clrf    uc_state

    ;; Initialize from EEPROM.
    eeprom_read16_lf    uc_ee_temp, w16
    tempc_set16         w16
    eeprom_read_lw      uc_ee_air
    airpump_setw
    endif

    ifdef section_idle
    uc_idle
    endif
