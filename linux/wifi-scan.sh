#!/bin/sh
# Run from the RAM-based Linux system with sysfs mounted.
set -eu
find_device() {
    for usb in /sys/bus/usb/devices/*; do
        [ -r "$usb/idVendor" ] && [ -r "$usb/idProduct" ] || continue
        if [ "$(cat "$usb/idVendor")" = 0bda ] && [ "$(cat "$usb/idProduct")" = "$1" ]; then
            readlink -f "$usb"
            return 0
        fi
    done
    return 1
}

if ! wifi=$(find_device c820); then
    cdrom=$(find_device 1a2b) || { echo "Dongle Realtek absent."; exit 1; }
    usb_modeswitch -v 0x0bda -p 0x1a2b -b "$(cat "$cdrom/busnum")" -g "$(cat "$cdrom/devnum")" -K || echo "Verification de la reenumeration..."
    wifi=$cdrom
fi

for elapsed in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15; do
    if [ -r "$wifi/idProduct" ] && [ "$(cat "$wifi/idProduct")" = c820 ] &&
       [ "$(cat "$wifi/idVendor")" = 0bda ]; then
        for net in "$wifi":*/net/*; do
            [ -d "$net/phy80211" ] || continue
            iface=${net##*/}
            ip link set "$iface" up
            exec iw dev "$iface" scan
        done
    fi
    sleep 1
done
echo "Interface Wi-Fi indisponible. Verifier le pilote rtw88_8821cu et le firmware avec dmesg." >&2
exit 1
