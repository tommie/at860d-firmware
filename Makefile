AS = gpasm
CC = sdcc
LD = gplink

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
	$(AS) $(ASFLAGS) -c -o $@ $<
