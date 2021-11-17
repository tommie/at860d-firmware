    ;; Low-level LED display driver module.
    ;;
    ;; Digits (CA): RC1-7
    ;; Segments: RD0-7

    ifdef module_led
#undefine module_led
    endif
#define module_led
    ifndef module_stdlib
    error "stdlib module is a dependency of led"
    endif

    ifdef section_code
ledca_blank     macro
    movlw   ~0xFE
    andwf   PORTC, F
    endm

ledseg_set      macro
    movwf   PORTD
    endm

    ;; ledca_blank must be called first.
ledca_setf_irq  macro   file
    movlw   HIGH ledca_table_irq
    movwf   PCLATH
    movf    file, W
    call    ledca_table_irq
    iorwf   PORTC, F
    endm

ledca_table_irq:
    andlw   0x07
    table_jump  tblirq
    ;; LED1:1
    dt      0x20, 0x40, 0x80
    ;; LED2:1
    dt      0x04, 0x08, 0x10
    ;; LED3:1
    dt      0x02
    dt      0

ledseg_translate    macro file
    movlw   HIGH ledseg_table
    movwf   PCLATH
    movf    file, W
    call    ledseg_table
    endm

LSA     EQU     ~0x80
LSB     EQU     ~0x20
LSC     EQU     ~0x08
LSD     EQU     ~0x04
LSE     EQU     ~0x02
LSF     EQU     ~0x40
LSG     EQU     ~0x10
LSDP    EQU     ~0x01

LS2A    EQU     ~0x80
LS2B    EQU     ~0x01
LS2C    EQU     ~0x20
LS2D    EQU     ~0x04
LS2E    EQU     ~0x08
LS2F    EQU     ~0x40
LS2G    EQU     ~0x02
LS2DP   EQU     ~0x10

ledseg_table:
    andlw   0x0F
    table_jump  tbl
    dt      LSA & LSB & LSC & LSD & LSE & LSF
    dt      LSB & LSC
    dt      LSA & LSB & LSD & LSE & LSG
    dt      LSA & LSB & LSC & LSD & LSG
    dt      LSB & LSC & LSF & LSG
    dt      LSA & LSC & LSD & LSF & LSG
    dt      LSA & LSC & LSD & LSE & LSF & LSG
    dt      LSA & LSB & LSC
    dt      LSA & LSB & LSC & LSD & LSE & LSF & LSG
    dt      LSA & LSB & LSC & LSD & LSF & LSG
    dt      0xFF
    dt      0xFF
    dt      LS2A & LS2D & LS2E & LS2F & LS2DP   ;; "*C" on aux LED.
    dt      0xFF
    dt      0xFF
    dt      0xFF                                ;; Blank
    endif

    ifdef section_init
    ;; Segments
    movlw   ~0xFE
    andwf   PORTC, F
    banksel TRISC
    andwf   TRISC, F
    banksel PORTC

    ;; Digits (CA)
    clrw
    sublw   1
    movwf   PORTD
    banksel TRISD
    clrf    TRISD
    banksel PORTD
    endif
