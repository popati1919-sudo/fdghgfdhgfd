# CX Internet - sonde USB (etape 1). A compiler avec le SDK Ndless (voir LISEZMOI.md).
GCC = nspire-gcc
LD  = nspire-ld
GENZEHN = genzehn

GCCFLAGS = -Wall -W -marm -Os
ZEHNFLAGS = --name "CX Internet USB Probe" --author "CX Internet" --notice "Sonde USB en lecture seule" --ndless-min 36

all: usbprobe.tns

src/usbprobe.o: src/usbprobe.c src/usbprobe_decode.h
	$(GCC) $(GCCFLAGS) -c $< -o $@

usbprobe.elf: src/usbprobe.o
	$(LD) $^ -o $@

usbprobe.tns: usbprobe.elf
	$(GENZEHN) --input $< --output $@.zehn $(ZEHNFLAGS)
	make-prg $@.zehn $@
	rm -f $@.zehn

test:
	gcc -std=c99 -Wall -Wextra -Wpedantic -o tests/test_decode tests/test_decode.c && ./tests/test_decode

clean:
	rm -f src/*.o *.elf *.tns *.zehn tests/test_decode
