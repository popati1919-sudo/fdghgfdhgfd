# CX Internet - sondes USB. A compiler avec le SDK Ndless.
GCC = nspire-gcc
LD  = nspire-ld
GENZEHN = genzehn
GCCFLAGS = -Wall -W -marm -Os

all: usbprobe10.tns

src/usbprobe10_safe.o: src/usbprobe10_safe.c
	$(GCC) $(GCCFLAGS) -c $< -o $@

usbprobe10_safe.elf: src/usbprobe10_safe.o
	$(LD) $^ -o $@

usbprobe10.tns: usbprobe10_safe.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB Probe 10" --author "CX Internet" --notice "Inventaire passif API USB Ndless, sans transfert" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

test:
	gcc -std=c99 -Wall -Wextra -Wpedantic -o tests/test_decode tests/test_decode.c && ./tests/test_decode
	gcc -std=c99 -Wall -Wextra -Wpedantic -o tests/test_usb_descriptors tests/test_usb_descriptors.c && ./tests/test_usb_descriptors
	gcc -std=c99 -Wall -Wextra -Wpedantic -o tests/test_rtl8821cu tests/test_rtl8821cu.c && ./tests/test_rtl8821cu

clean:
	rm -f src/*.o *.elf *.tns *.zehn tests/test_decode tests/test_usb_descriptors tests/test_rtl8821cu
