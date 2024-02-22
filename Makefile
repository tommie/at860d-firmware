# #v(index) inside a macro causes "Symbol index not assigned a value."
# --mpasm-compatible should fix that, in at least v1.5.0.
AS = gpasm --mpasm-compatible
LD = gplink
GPVC = gpvc

asflags = -p p16f887 -r dec
ldflags = -m

# These optimizations are only available since gputils 1.5.0.
ldflags_gplink1_5 = -b 1 -p 1

.PHONY: all
all: at860d.hex

.PHONY: clean
clean:
	rm -f *.hex *.lst *.cod
	rm -f *.map *.o

at860d.hex: at860d.o

%.cod: %.hex
%.lst: %.hex
%.hex: %.o
	$(LD) $(ldflags) $(LDFLAGS) -o $@ $^
	@$(GPVC) $(basename $@).cod | awk -Wnon-decimal-data '/using ROM 0021.. / { print "EEDATA:", 1+("0x" $$5)-("0x" $$3); } /using ROM 002/ { next; } /using ROM 0/ { prog += 1+("0x" $$5)-("0x" $$3); } END { print "Program:", prog; }'

%.o: %.asm *.inc
	@echo $(AS) $(asflags) $(ASFLAGS) -c -o $@ $<
	@$(AS) $(asflags) $(ASFLAGS) -c -o $@ $< >"$(dir $@)/.$(notdir $@).log" ;\
		status=$$? ;\
		grep -Fv 'Symbol index not assigned a value.' "$(dir $@)/.$(notdir $@).log" || : ;\
		rm "$(dir $@)/.$(notdir $@).log" ;\
		exit $$status
