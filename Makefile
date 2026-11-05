# CX Internet - sondes USB. A compiler avec le SDK Ndless.
GCC = nspire-gcc
LD  = nspire-ld
GENZEHN = genzehn
GCCFLAGS = -Wall -W -marm -Os

all: usbprobe11.tns

src/usbprobe11_abi.o: src/usbprobe11_abi.c
	$(GCC) $(GCCFLAGS) -c $< -o $@

usbprobe11_abi.elf: src/usbprobe11_abi.o
	$(LD) $^ -o $@

usbprobe11.tns: usbprobe11_abi.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB Probe 11" --author "CX Internet" --notice "Audit ABI USB passif, sans action materielle" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

test:
	gcc -std=c99 -Wall -Wextra -Wpedantic -o tests/test_decode tests/test_decode.c && ./tests/test_decode
	gcc -std=c99 -Wall -Wextra -Wpedantic -o tests/test_usb_descriptors tests/test_usb_descriptors.c && ./tests/test_usb_descriptors
	gcc -std=c99 -Wall -Wextra -Wpedantic -o tests/test_rtl8821cu tests/test_rtl8821cu.c && ./tests/test_rtl8821cu

clean:
	rm -f src/*.o *.elf *.tns *.zehn tests/test_decode tests/test_usb_descriptors tests/test_rtl8821cu
