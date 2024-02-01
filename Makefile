# #v(index) inside a macro causes "Symbol index not assigned a value."
# --mpasm-compatible should fix that, in at least v1.5.0.
AS = gpasm --mpasm-compatible
CC = sdcc
LD = gplink
SHELL = bash  # For pipefail

ASFLAGS = -p p16f887 -r dec
LDFLAGS = -m

.PHONY: all
all: at860d.hex

.PHONY: clean
clean:
	rm -f *.hex *.lst *.cod
	rm -f *.o

at860d.hex: at860d.o

%.hex %.lst: %.o
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.asm *.inc
	set -o pipefail
	$(AS) $(ASFLAGS) -c -o $@ $< | grep -Fv 'Symbol index not assigned a value.'
