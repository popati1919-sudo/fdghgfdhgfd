# CX Internet - sondes USB. A compiler avec le SDK Ndless (voir LISEZMOI.md).
GCC = nspire-gcc
LD  = nspire-ld
GENZEHN = genzehn

GCCFLAGS = -Wall -W -marm -Os

all: rtl8821cu_core.tns usbprobe.tns usbprobe1b.tns usbprobe3.tns usbprobe4.tns usbprobe5.tns usbprobe6.tns usbprobe7.tns usbprobe8.tns usbprobe9.tns usbprobe15.tns usbprobe16.tns usbprobe17.tns usbprobe18.tns rtl8821cu_core.tns rtl8821cu_core04.tns rtl8821cu_wifi05.tns rtl8821cu_attach19.tns rtl8821cu_modeswitch20.tns

src/%.o: src/%.c src/usbprobe_decode.h src/usb_spy.h src/usb_descriptors.h
	$(GCC) $(GCCFLAGS) -c $< -o $@

src/usb_spy.o: src/usb_spy.S
	$(GCC) $(GCCFLAGS) -c $< -o $@

src/usbprobe8.o: src/usbprobe8.c src/usb_spy8.h src/usbprobe_decode.h
	$(GCC) $(GCCFLAGS) -c $< -o $@

src/usbprobe9.o: src/usbprobe9.c src/usb_spy8.h src/usbprobe_decode.h
	$(GCC) $(GCCFLAGS) -c $< -o $@

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
	gcc -std=c99 -Wall -Wextra -Wpedantic -o tests/test_scsi_eject tests/test_scsi_eject.c && ./tests/test_scsi_eject
	gcc -std=c99 -Wall -Wextra -Wpedantic -o tests/test_usb_bot_gate tests/test_usb_bot_gate.c && ./tests/test_usb_bot_gate
	gcc -std=c99 -Wall -Wextra -Wpedantic -o tests/test_rtl8821cu_audit tests/test_rtl8821cu_audit.c && ./tests/test_rtl8821cu_audit
	gcc -std=c99 -Wall -Wextra -Wpedantic -o tests/test_rtl8821cu_protocol tests/test_rtl8821cu_protocol.c && ./tests/test_rtl8821cu_protocol
	gcc -std=c99 -Wall -Wextra -Wpedantic -o tests/test_rtl8821cu_core tests/test_rtl8821cu_core.c && ./tests/test_rtl8821cu_core

src/usbprobe15.o: src/usbprobe15.c src/scsi_eject.h
	$(GCC) $(GCCFLAGS) -c $< -o $@

usbprobe15.elf: src/usbprobe15.o
	$(LD) $^ -o $@

usbprobe15.tns: usbprobe15.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB Probe 15" --author "CX Internet" --notice "Validation SCSI BOT sans transfert USB" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

test-arm:
	arm-linux-gnueabihf-gcc -marm -std=gnu11 -Wall -Wextra -O1 -static -o tests/test_spy tests/test_spy.c src/usb_spy.S && qemu-arm tests/test_spy

clean:
	rm -f src/*.o *.elf *.tns *.zehn tests/test_decode tests/test_usb_descriptors tests/test_rtl8821cu tests/test_scsi_eject tests/test_spy

src/usbprobe16.o: src/usbprobe16.c src/usb_bot_gate.h
	$(GCC) $(GCCFLAGS) -c $< -o $@

usbprobe16.elf: src/usbprobe16.o
	$(LD) $^ -o $@

usbprobe16.tns: usbprobe16.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB Probe 16" --author "CX Internet" --notice "Garde compatibilite BOT sans transfert USB" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

src/usbprobe17.o: src/usbprobe17.c src/rtl8821cu_audit.h
	$(GCC) $(GCCFLAGS) -c $< -o $@

usbprobe17.elf: src/usbprobe17.o
	$(LD) $^ -o $@

usbprobe17.tns: usbprobe17.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB Probe 17" --author "CX Internet" --notice "Audit RTL8821CU sans transfert USB" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

src/usbprobe18.o: src/usbprobe18.c src/rtl8821cu_protocol.h
	$(GCC) $(GCCFLAGS) -c $< -o $@

usbprobe18.elf: src/usbprobe18.o
	$(LD) $^ -o $@

usbprobe18.tns: usbprobe18.elf
	$(GENZEHN) --input $< --output $@.zehn --name "CX Internet USB Probe 18" --author "CX Internet" --notice "Audit firmware et protocole RTL8821C hors-ligne" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

src/rtl8821cu_core_app.o: src/rtl8821cu_core_app.c src/rtl8821cu_core.h
	$(GCC) $(GCCFLAGS) -c $< -o $@

rtl8821cu_core.elf: src/rtl8821cu_core_app.o
	$(LD) $^ -o $@

rtl8821cu_core.tns: rtl8821cu_core.elf
	$(GENZEHN) --input $< --output $@.zehn --name "RTL8821CU Driver Core" --author "CX Internet" --notice "Noyau pilote hors ligne, sans E/S USB" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

src/rtl8821cu_core04_app.o: src/rtl8821cu_core04_app.c src/rtl8821cu_core.h
	$(GCC) $(GCCFLAGS) -c $< -o $@

rtl8821cu_core04.elf: src/rtl8821cu_core04_app.o
	$(LD) $^ -o $@

rtl8821cu_core04.tns: rtl8821cu_core04.elf
	$(GENZEHN) --input $< --output $@.zehn --name "RTL8821CU Driver Core 0.4" --author "CX Internet" --notice "Verification ABI USB Ndless sans E/S materielle" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

src/rtl8821cu_wifi05_app.o: src/rtl8821cu_wifi05_app.c src/rtl8821cu_core.h
	$(GCC) $(GCCFLAGS) -c $< -o $@

rtl8821cu_wifi05.elf: src/rtl8821cu_wifi05_app.o
	$(LD) $^ -o $@

rtl8821cu_wifi05.tns: rtl8821cu_wifi05.elf
	$(GENZEHN) --input $< --output $@.zehn --name "RTL8821CU Wi-Fi 0.5" --author "CX Internet" --notice "Interface interactive; scan bloque jusqu'au pilote USB" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

src/rtl8821cu_attach_app.o: src/rtl8821cu_attach_app.c
	$(GCC) $(GCCFLAGS) -c $< -o $@

rtl8821cu_attach19.elf: src/rtl8821cu_attach_app.o
	$(LD) $^ -o $@

rtl8821cu_attach19.tns: rtl8821cu_attach19.elf
	$(GENZEHN) --input $< --output $@.zehn --name "RTL8821CU Attach Probe 0.1" --author "CX Internet" --notice "Attachement USB interface en lecture seule" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn

src/rtl8821cu_modeswitch_app.o: src/rtl8821cu_modeswitch_app.c src/scsi_eject.h
	$(GCC) $(GCCFLAGS) -c $< -o $@

rtl8821cu_modeswitch20.elf: src/rtl8821cu_modeswitch_app.o
	$(LD) $^ -o $@

rtl8821cu_modeswitch20.tns: rtl8821cu_modeswitch20.elf
	$(GENZEHN) --input $< --output $@.zehn --name "RTL8821CU Mode Switch 0.1" --author "CX Internet" --notice "Ejection SCSI BOT unique du mode CD Realtek" --ndless-min 36
	make-prg $@.zehn $@
	rm -f $@.zehn
