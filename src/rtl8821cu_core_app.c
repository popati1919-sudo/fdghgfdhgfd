/* RTL8821CU Driver Core - TI-Nspire CX
 * This executable links the portable driver core into a native Ndless .tns.
 * It validates descriptor parsing/state progression offline; no USB I/O yet.
 */
#include <os.h>
#include <libndls.h>
#include <stdio.h>
#include "rtl8821cu_core.h"

int main(void)
{
    /* Synthetic descriptor layout matching the Windows USBTreeView profile:
       interface 2, vendor-specific class FF, bulk IN 84 and bulk OUT 05/06. */
    static const uint8_t cfg[] = {
        9,2,48,0,1,1,0,0x80,50,
        9,4,2,0,3,0xff,0xff,0xff,0,
        7,5,0x84,2,0,2,0,
        7,5,0x05,2,0,2,0,
        7,5,0x06,2,0,2,0
    };
    CxiRtlDeviceProfile profile;
    CxiRtlStage stage = CXI_RTL_STAGE_DETACHED;
    CxiRtlCoreStatus parse_status;
    char report[900];

    parse_status = cxi_rtl_parse_config(cfg, sizeof(cfg), &profile);
    if (parse_status != CXI_RTL_CORE_OK) {
        snprintf(report, sizeof(report),
            "RTL8821CU DRIVER CORE\n\nErreur analyse descripteurs: %d",
            (int)parse_status);
        show_msgbox("RTL8821CU Core", report);
        return 1;
    }

    (void)cxi_rtl_advance_stage(&stage, CXI_RTL_STAGE_ENUMERATED);
    (void)cxi_rtl_advance_stage(&stage, CXI_RTL_STAGE_INTERFACE_READY);

    snprintf(report, sizeof(report),
        "RTL8821CU DRIVER CORE\n"
        "Version: noyau portable 0.1\n\n"
        "Cible: %04X:%04X\n"
        "Interface: %u / classe %02X\n"
        "Endpoints analyses: %u\n"
        "Bulk IN: %u\n"
        "Bulk OUT: %u\n\n"
        "Analyse descripteurs: OK\n"
        "Machine d'etats: ENUMERE -> INTERFACE\n\n"
        "USB Ndless: non raccorde\n"
        "Firmware: non charge\n"
        "Scan Wi-Fi: non lance\n\n"
        "Ce programme embarque le Driver Core,\n"
        "mais ne communique pas encore avec la cle.",
        (unsigned)CXI_RTL8821CU_VID, (unsigned)CXI_RTL8821CU_PID,
        (unsigned)profile.wifi.number, (unsigned)profile.wifi.klass,
        (unsigned)profile.wifi.endpoint_count,
        (unsigned)profile.bulk_in_count, (unsigned)profile.bulk_out_count);
    show_msgbox("RTL8821CU Core", report);
    return 0;
}
