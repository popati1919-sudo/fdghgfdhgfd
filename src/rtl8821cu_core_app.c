/* RTL8821CU Driver Core 0.2 - TI-Nspire CX
 * Offline descriptor/profile validator.
 *
 * IMPORTANT: the profile below is copied from the user's Windows USBTreeView
 * report. It is NOT read from the calculator or the attached USB adapter.
 * Ndless user-space USB enumeration/endpoint I/O is not connected here.
 */
#include <os.h>
#include <libndls.h>
#include <stdio.h>
#include "rtl8821cu_core.h"

int main(void)
{
    /* Offline reference profile based on Windows USBTreeView, not live USB. */
    static const uint8_t cfg[] = {
        9,2,39,0,1,1,0,0x80,50,
        9,4,2,0,3,0xff,0xff,0xff,0,
        7,5,0x84,2,0,2,0,
        7,5,0x05,2,0,2,0,
        7,5,0x06,2,0,2,0
    };
    CxiRtlDeviceProfile profile;
    CxiRtlCoreStatus parse_status;
    char report[900];

    parse_status = cxi_rtl_parse_config(cfg, sizeof(cfg), &profile);
    if (parse_status != CXI_RTL_CORE_OK) {
        snprintf(report, sizeof(report),
            "RTL8821CU DRIVER CORE 0.2\n\n"
            "Erreur profil hors ligne: %d", (int)parse_status);
        show_msgbox("RTL8821CU Core 0.2", report);
        return 1;
    }

    snprintf(report, sizeof(report),
        "RTL8821CU DRIVER CORE\n"
        "Version: noyau portable 0.2\n\n"
        "SOURCE: profil Windows hors ligne\n"
        "Cible de reference: %04X:%04X\n"
        "Interface Windows: %u / classe %02X\n"
        "Endpoints du profil: %u\n"
        "Bulk IN: %u (0x84)\n"
        "Bulk OUT: %u (0x05, 0x06)\n\n"
        "Analyse du profil: OK\n"
        "USB reel sur la TI: NON VERIFIE\n"
        "API USB Ndless: non raccordee\n"
        "Machine d'etats: DETACHED\n\n"
        "Firmware: non charge\n"
        "Scan Wi-Fi: non lance\n\n"
        "Cette version valide le profil Windows uniquement.\n"
        "Aucune commande USB n'est envoyee a la cle.",
        (unsigned)CXI_RTL8821CU_VID, (unsigned)CXI_RTL8821CU_PID,
        (unsigned)profile.wifi.number, (unsigned)profile.wifi.klass,
        (unsigned)profile.wifi.endpoint_count,
        (unsigned)profile.bulk_in_count, (unsigned)profile.bulk_out_count);
    show_msgbox("RTL8821CU Core 0.2", report);
    return 0;
}
