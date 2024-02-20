    ;; Handles the knob in a sensible way.
    ;;
    ;; The original firmware sets a temp/air value as soon as the knob moves.
    ;; This can cause the value to jump around quite a lot, and that's
    ;; especially bad if the knob was left in a high position, and then
    ;; you want to change the temperature to something low.
    ;;
    ;; Instead, we wait for the user to pass the current value, and only then
    ;; do we adjust the value as normal. This avoids jumping values at the
    ;; expense of having a confusing dead zone. This means the caller must
    ;; deactivate the knob whenever the underlying value is changed another way.
    ;;
    ;; Another way would have been to use it incrementally, but when
    ;; it reaches an end, we disable it until the user turns it back to
    ;; the center. Not better.
    ;;
    ;; A third way would be to use it as a "jogging wheel" where the center
    ;; is zero (including a fairly wide dead zone) and turning it makes
    ;; the value go more positive or negative. This doesn't seem very different
    ;; from buttons with increasing repeat frequency. Not better.
    ;;
    ;; The ADC value will flicker when the knob is between values, which it
    ;; often is. The simplest solution is to create a small hysteresis. The aim
    ;; being to make it as small as possible to avoid making the trail feel
    ;; like a dead zone (even if it is.)
    ;;
    ;; Using a rotary encoder would have been so much more sensible, since
    ;; presets can change the values without the potentiometer being tied to it.

    ifdef module_knob
#undefine module_knob
    endif
#define module_knob
    ifndef module_adc
    error "adc module is a dependency of knob"
    endif

    ifdef section_udata
KNOB_DISABLED   equ 0xFF

    ;; knob_state bits
KNOB_ACTIVE equ 0
KNOB_NEG    equ 1
KNOB_POS    equ 2
KNOB_EQ     equ 3

knob_index  res 1
knob_state  res 1
knob_hystl  res 2               ; Low end of hysteresis window, in ADC units.
    endif

    ifdef section_code
    ;; Sets the current channel the knob is connected to.
    ;; Use KNOB_DISABLED to disconnect the knob.
    ;; Does nothing if `index` is already the current channel.
    ;; The knob starts out deactivated.
knob_select macro   index
    bsf     STATUS, DC
    knob_skip_if_current_channel    index
    bcf     STATUS, DC
    selbank knob_state
    btfss   STATUS, DC
    clrf    knob_state

    movlw   index
    selbnkc knob_index
    movwf   knob_index
    endm

    ;; If the current channel is `index`, the knob is deactivated.
    ;; This must be called if the underlying value was changed by
    ;; something other than the knob, or knob_idle will overwrite it.
knob_deactivate macro   index
    local   mend

    ;; if (knob_index != index) goto mend
    pagesel mend
    knob_skip_if_current_channel    index
    goto    mend

    selbnkc knob_state
    clrf    knob_state
mend:
    endm

    ;; Same as `knob_deactivate`, but for any current channel.
knob_deactivate_all macro
    selbank knob_state
    clrf    knob_state
    endm

knob_skip_if_current_channel    macro   index
    selbank knob_index
    movf    knob_index, W
    sublw   index
    btfss   STATUS, Z
    endm

    ;; Code to be run on idle for knob channel `index`.
    ;; `index` must be less than 0xFF, but is otherwise uninterpreted.
    ;; `size` is 1 for 8-bit values and 2 for 16-bit values.
    ;; `setmacro` sets the value in W for 8-bit and takes a source file in 16-bit mode.
knob_idle   macro   index, size, lit_min, lit_max, file, setmacro
    local   is_active
    local   mend

    if      (size) < 1 || (size) > 2
    error   "size must be in [1, 2]"
    endif
    if      (lit_max) >= 1 << (8*(size))
    error   "lit_max cannot be represented by size bytes"
    endif
    if      (lit_min) >= (lit_max)
    error   "lit_min must be less than lit_max"
    endif
    ifndef  KNOB_HYSTERESIS
    error   "KNOB_HYSTERESIS is not defined"
    else
    if      KNOB_HYSTERESIS >= 1 << 8
    error   "Assertion failed: KNOB_HYSTERESIS < 256"
    endif
    endif

    ;; if (knob_index != index) goto mend
    pagesel     mend
    knob_skip_if_current_channel    index
    goto        mend

    ;; w16 = (lit_max - lit_min) * adc_knob_value/2^10 + lit_min
    mov16ff     adc_knob_value, w16
    ufpmul16lf  (lit_max) - (lit_min), w16, 10
    add16lf     lit_min, w16

    ;; w16 -= file
    if          size == 1
    sub0816ff   file, w16
    else
    sub16ff     file, w16
    endif

    ;; knob_state.eq = w16 == 0
    zero16f w16
    selbnkc knob_state
    bcf     knob_state, KNOB_EQ
    btfsc   STATUS, Z
    bsf     knob_state, KNOB_EQ

    ;; if (knob_state.active) goto is_active
    pagesel is_active
    selbnkc knob_state
    btfsc   knob_state, KNOB_ACTIVE
    goto    is_active

    ;; knob_state.neg ||= w16 < 0
    btfsc   w16 + 1, 7
    bsf     knob_state, KNOB_NEG

    ;; knob_state.pos ||= w16 >= 0
    btfss   w16 + 1, 7
    bsf     knob_state, KNOB_POS

    ;; knob_state.active ||= knob_state.eq || (knob_state.neg && knob_state.pos)
    btfsc   knob_state, KNOB_EQ
    bsf     knob_state, KNOB_ACTIVE
    movf    knob_state, W
    andlw   (1 << KNOB_NEG) | (1 << KNOB_POS)
    xorlw   (1 << KNOB_NEG) | (1 << KNOB_POS)
    btfsc   STATUS, Z
    bsf     knob_state, KNOB_ACTIVE

    ;; if (!knob_state.active) goto mend
    pagesel mend
    btfss   knob_state, KNOB_ACTIVE
    goto    mend

    ;; knob_hystl = adc_knob_value - (HYSTERESIS * 3 + 3) / 4
    mov16lf -((KNOB_HYSTERESIS) * 3 + 3) / 4, knob_hystl
    add16ff adc_knob_value, knob_hystl

    ;; fall through

is_active:
    ;; if (knob_state.eq) goto mend
    pagesel mend
    selbnkc knob_state
    btfsc   knob_state, KNOB_EQ
    goto    mend

    ;; Clear state bits to be recomputed.
    selbnkc knob_state
    movlw   ~((1 << KNOB_NEG) | (1 << KNOB_POS))
    andwf   knob_state, F

    ;; knob_state.neg = adc_knob_value < knob_hystl
    gts16ff knob_hystl, adc_knob_value
    selbnkc knob_state
    btfsc   STATUS, C
    bsf     knob_state, KNOB_NEG

    ;; knob_state.pos = adc_knob_value > knob_hystl + (HYSTERESIS * 3 + 1) / 2 + 1
    ;;
    ;; Plus 1 for the current value.
    add16lf ((KNOB_HYSTERESIS) * 3 + 1) / 2 + 1, knob_hystl
    gts16ff adc_knob_value, knob_hystl
    selbnkc knob_state
    btfsc   STATUS, C
    bsf     knob_state, KNOB_POS
    add16lf -(((KNOB_HYSTERESIS) * 3 + 1) / 2 + 1), knob_hystl

    ;; if (!knob_state.neg && !knob_state.pos) goto mend
    pagesel mend
    movf    knob_state, W
    andlw   (1 << KNOB_NEG) | (1 << KNOB_POS)
    btfsc   STATUS, Z
    goto    mend

    ;; w16 += file
    if          size == 1
    add0816ff   file, w16
    else
    add16ff     file, w16
    endif

    if          size == 1
    movf        w16, W
    setmacro
    else
    setmacro    w16
    endif

    ;; We use HYSTERESIS/2 in the direction the user was previously going in,
    ;; and HYSTERESIS in the opposite direction.
    ;;
    ;; knob_hystl = adc_knob_value - (knob_state.pos ? HYSTERESIS : HYSTERESIS / 2)
    selbank     knob_state
    movlw       -(KNOB_HYSTERESIS)
    btfss       knob_state, KNOB_POS
    movlw       -(KNOB_HYSTERESIS) / 2
    selbnkc     knob_hystl
    movwf       knob_hystl
    selbnkc     knob_hystl + 1
    clrf        knob_hystl + 1
    comf        knob_hystl + 1, F
    add16ff     adc_knob_value, knob_hystl

mend:
    endm
    endif

    ifdef section_init
    movlw   KNOB_DISABLED
    selbank knob_index
    movwf   knob_index
    selbnkc knob_state
    clrf    knob_state
    endif
