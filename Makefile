# CX Internet - sondes USB. A compiler avec le SDK Ndless (voir LISEZMOI.md).
GCC = nspire-gcc
LD  = nspire-ld
GENZEHN = genzehn

GCCFLAGS = -Wall -W -marm -Os

all: usbprobe9.tns

src/%.o: src/%.c src/usbprobe_decode.h src/usb_spy.h src/usb_descriptors.h
	$(GCC) $(GCCFLAGS) -c $< -o $@

src/usb_spy.o: src/usb_spy.S
	$(GCC) $(GCCFLAGS) -c $< -o $@

src/usbprobe8.o: src/usbprobe8.c src/usb_spy8.h src/usbprobe_decode.h
	$(GCC) $(GCCFLAGS) -c $< -o $@

src/usbprobe9_safe.o: src/usbprobe9_safe.c
	$(GCC) $(GCCFLAGS) -c $< -o $@

usbprobe9_safe.elf: src/usbprobe9_safe.o
	$(LD) $^ -o $@

usbprobe9.tns: usbprobe9_safe.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB Probe 9" --author "CX Internet" --notice "Reference endpoints RTL8821CU sans transfert USB" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

src/usb_spy9.o: src/usb_spy9.S
	$(GCC) $(GCCFLAGS) -c $< -o $@

src/usb_spy8.o: src/usb_spy8.S
	$(GCC) $(GCCFLAGS) -c $< -o $@

usbprobe.elf: src/usbprobe.o
	$(LD) $^ -o $@

usbprobe1b.elf: src/usbprobe1b.o src/usb_spy.o
	$(LD) $^ -o $@

usbprobe3.elf: src/usbprobe3.o src/usb_spy.o
	$(LD) $^ -o $@

usbprobe4.elf: src/usbprobe4.o src/usb_spy.o
	$(LD) $^ -o $@

usbprobe5.elf: src/usbprobe5.o src/usb_spy.o
	$(LD) $^ -o $@

usbprobe6.elf: src/usbprobe6.o src/usb_spy.o
	$(LD) $^ -o $@

usbprobe7.elf: src/usbprobe7.o src/usb_spy.o
	$(LD) $^ -o $@

usbprobe8.elf: src/usbprobe8.o src/usb_spy8.o
	$(LD) $^ -o $@

usbprobe9.elf: src/usbprobe9.o src/usb_spy9.o
	$(LD) $^ -o $@

usbprobe.tns: usbprobe.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB Probe" --author "CX Internet" --notice "Sonde USB en lecture seule" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

usbprobe1b.tns: usbprobe1b.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB 1b" --author "CX Internet" --notice "Lit le VID:PID (pilote espion)" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

usbprobe3.tns: usbprobe3.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB Probe 3" --author "CX Internet" --notice "Lit les descripteurs USB" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

usbprobe4.tns: usbprobe4.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB Probe 4" --author "CX Internet" --notice "Inspecte interfaces et endpoints en lecture seule" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

usbprobe5.tns: usbprobe5.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB Probe 5" --author "CX Internet" --notice "Capture USB unique sans relecture" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

usbprobe6.tns: usbprobe6.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB Probe 6" --author "CX Internet" --notice "Diagnostic VID:PID sans commande USB" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

usbprobe7.tns: usbprobe7.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB Probe 7" --author "CX Internet" --notice "Suivi VID:PID sans commande USB" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

usbprobe8.tns: usbprobe8.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB Probe 8" --author "CX Internet" --notice "Nombre d interfaces USB sans dereferencement" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

usbprobe9.tns: usbprobe9.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB Probe 9" --author "CX Internet" --notice "Snapshot USB borne sans pointeur imbrique" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

test:
	gcc -std=c99 -Wall -Wextra -Wpedantic -o tests/test_decode tests/test_decode.c && ./tests/test_decode
	gcc -std=c99 -Wall -Wextra -Wpedantic -o tests/test_usb_descriptors tests/test_usb_descriptors.c && ./tests/test_usb_descriptors
	gcc -std=c99 -Wall -Wextra -Wpedantic -o tests/test_rtl8821cu tests/test_rtl8821cu.c && ./tests/test_rtl8821cu

test-arm:
	arm-linux-gnueabihf-gcc -marm -std=gnu11 -Wall -Wextra -O1 -static -o tests/test_spy tests/test_spy.c src/usb_spy.S && qemu-arm tests/test_spy

clean:
	rm -f src/*.o *.elf *.tns *.zehn tests/test_decode tests/test_usb_descriptors tests/test_rtl8821cu tests/test_spy

src/usbwifi_installer.o: src/usbwifi_installer.c
	$(GCC) $(GCCFLAGS) -c $< -o $@

usbwifi-installer.elf: src/usbwifi_installer.o
	$(LD) $^ -o $@

usbwifi-installer.tns: usbwifi-installer.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet Wi-Fi Installer" --author "CX Internet" --notice "Installateur preparatoire RTL8821CU" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn
