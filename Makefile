# CX Internet - sondes USB. A compiler avec le SDK Ndless.
GCC = nspire-gcc
LD  = nspire-ld
GENZEHN = genzehn
GCCFLAGS = -Wall -W -marm -Os

all: usbprobe14.tns

src/usbprobe11_abi.o: src/usbprobe11_abi.c
	$(GCC) $(GCCFLAGS) -c $< -o $@

src/usbprobe12_abi.o: src/usbprobe12_abi.c
	$(GCC) $(GCCFLAGS) -std=gnu99 -c $< -o $@

usbprobe12_abi.elf: src/usbprobe12_abi.o
	$(LD) $^ -o $@

usbprobe12.tns: usbprobe12_abi.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB Probe 12" --author "CX Internet" --notice "Validation ABI USB compile-time uniquement" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

src/usbprobe13_match.o: src/usbprobe13_match.c
	$(GCC) $(GCCFLAGS) -std=gnu99 -c $< -o $@

usbprobe13_match.elf: src/usbprobe13_match.o
	$(LD) $^ -o $@

usbprobe13.tns: usbprobe13_match.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB Probe 13" --author "CX Internet" --notice "Test logiciel match RTL8821CU sans acces USB" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

src/usbprobe14_endpoints.o: src/usbprobe14_endpoints.c src/usb_descriptors.h src/rtl8821cu.h
	$(GCC) $(GCCFLAGS) -std=gnu99 -Isrc -c $< -o $@

usbprobe14_endpoints.elf: src/usbprobe14_endpoints.o
	$(LD) $^ -o $@

usbprobe14.tns: usbprobe14_endpoints.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB Probe 14" --author "CX Internet" --notice "Audit statique endpoints RTL8821CU" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

test:
	gcc -std=c99 -Wall -Wextra -Wpedantic -o tests/test_decode tests/test_decode.c && ./tests/test_decode
	gcc -std=c99 -Wall -Wextra -Wpedantic -o tests/test_usb_descriptors tests/test_usb_descriptors.c && ./tests/test_usb_descriptors
	gcc -std=c99 -Wall -Wextra -Wpedantic -o tests/test_rtl8821cu tests/test_rtl8821cu.c && ./tests/test_rtl8821cu

clean:
	rm -f src/*.o *.elf *.tns *.zehn tests/test_decode tests/test_usb_descriptors tests/test_rtl8821cu
