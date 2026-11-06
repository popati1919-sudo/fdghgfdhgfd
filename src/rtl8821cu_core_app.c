/* RTL8821CU Driver Core 0.3 - TI-Nspire CX
 * Ndless USB syscall availability audit.
 *
 * This checks only whether the SDK syscall table exposes USB entry points.
 * It does NOT call USB functions, enumerate devices, claim an interface,
 * send requests, open pipes, transfer data, or load firmware.
 */
#include <os.h>
#include <libndls.h>
#include <stdio.h>
#include "rtl8821cu_core.h"

int main(void)
{
    static const uint8_t cfg[] = {
        9,2,39,0,1,1,0,0x80,50,
        9,4,2,0,3,0xff,0xff,0xff,0,
        7,5,0x84,2,0,2,0,
        7,5,0x05,2,0,2,0,
        7,5,0x06,2,0,2,0
    };
    CxiRtlDeviceProfile profile;
    CxiRtlCoreStatus parse_status;
    char report[1100];
    int have_desc = nl_hassyscall(usbd_get_device_descriptor);
    int have_iface = nl_hassyscall(usbd_device2interface_handle);
    int have_pipe = nl_hassyscall(usbd_open_pipe);
    int have_xfer = nl_hassyscall(usbd_alloc_xfer) &&
                    nl_hassyscall(usbd_setup_xfer) &&
                    nl_hassyscall(usbd_transfer);
    int have_control = nl_hassyscall(usbd_do_request);
    int have_register = nl_hassyscall(usb_register_driver);

    parse_status = cxi_rtl_parse_config(cfg, sizeof(cfg), &profile);
    if (parse_status != CXI_RTL_CORE_OK) {
        snprintf(report, sizeof(report),
            "RTL8821CU DRIVER CORE 0.3\n\nErreur profil: %d",
            (int)parse_status);
        show_msgbox("RTL8821CU Core 0.3", report);
        return 1;
    }

    snprintf(report, sizeof(report),
        "RTL8821CU DRIVER CORE 0.3\n"
        "AUDIT API USB Ndless\n\n"
        "Profil Windows hors ligne: OK\n"
        "Cible: %04X:%04X / interface %u\n"
        "Endpoints de reference: IN 0x84, OUT 0x05/0x06\n\n"
        "Presence appels systeme:\n"
        "Get device descriptor: %s\n"
        "Get interface handle: %s\n"
        "Open USB pipe: %s\n"
        "Alloc/setup/transfer: %s\n"
        "Control request: %s\n"
        "Register driver: %s\n\n"
        "AUCUN appel USB execute.\n"
        "Aucun handle materiel obtenu.\n"
        "Aucun transfert / firmware / scan.\n"
        "Resultat = disponibilite API seulement.",
        (unsigned)CXI_RTL8821CU_VID, (unsigned)CXI_RTL8821CU_PID,
        (unsigned)profile.wifi.number,
        have_desc ? "DISPONIBLE" : "ABSENT",
        have_iface ? "DISPONIBLE" : "ABSENT",
        have_pipe ? "DISPONIBLE" : "ABSENT",
        have_xfer ? "DISPONIBLE" : "ABSENT",
        have_control ? "DISPONIBLE" : "ABSENT",
        have_register ? "DISPONIBLE" : "ABSENT");
    show_msgbox("RTL8821CU Core 0.3", report);
    return 0;
}
