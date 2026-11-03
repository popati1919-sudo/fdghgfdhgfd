# CX Internet - sondes USB. A compiler avec le SDK Ndless (voir LISEZMOI.md).
GCC = nspire-gcc
LD  = nspire-ld
GENZEHN = genzehn

GCCFLAGS = -Wall -W -marm -Os

all: usbprobe.tns usbprobe1b.tns

src/%.o: src/%.c src/usbprobe_decode.h src/usb_spy.h
	$(GCC) $(GCCFLAGS) -c $< -o $@

src/usb_spy.o: src/usb_spy.S
	$(GCC) $(GCCFLAGS) -c $< -o $@

usbprobe.elf: src/usbprobe.o
	$(LD) $^ -o $@

usbprobe1b.elf: src/usbprobe1b.o src/usb_spy.o
	$(LD) $^ -o $@

usbprobe.tns: usbprobe.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB Probe" --author "CX Internet" --notice "Sonde USB en lecture seule" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

usbprobe1b.tns: usbprobe1b.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB 1b" --author "CX Internet" --notice "Lit le VID:PID (pilote espion)" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

# Tests sur PC (decodage). test-arm: code resident sous qemu-user ARM (optionnel).
test:
	gcc -std=c99 -Wall -Wextra -Wpedantic -o tests/test_decode tests/test_decode.c && ./tests/test_decode

test-arm:
	arm-linux-gnueabihf-gcc -marm -std=gnu11 -Wall -Wextra -O1 -static -o tests/test_spy tests/test_spy.c src/usb_spy.S && qemu-arm tests/test_spy

clean:
	rm -f src/*.o *.elf *.tns *.zehn tests/test_decode tests/test_spy
