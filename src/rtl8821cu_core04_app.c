/* RTL8821CU Driver Core 0.4 - TI-Nspire CX / Ndless
 * ABI gate: verifies USB syscall entry points needed for later enumeration.
 * No USB function is invoked; no device handle is fabricated.
 */
#include <os.h>
#include <libndls.h>
#include <stdio.h>
#include "rtl8821cu_core.h"

typedef struct { const char *name; int present; } ApiCheck;

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
    char report[1500];
    ApiCheck checks[] = {
        {"get device descriptor", nl_hassyscall(usbd_get_device_descriptor)},
        {"get config descriptor", nl_hassyscall(usbd_get_config_descriptor)},
        {"get interface descriptor", nl_hassyscall(usbd_get_interface_descriptor)},
        {"get endpoint descriptor", nl_hassyscall(usbd_get_endpoint_descriptor)},
        {"interface count", nl_hassyscall(usbd_interface_count)},
        {"interface handle", nl_hassyscall(usbd_device2interface_handle)},
        {"endpoint count", nl_hassyscall(usbd_endpoint_count)},
        {"open pipe", nl_hassyscall(usbd_open_pipe)},
        {"close pipe", nl_hassyscall(usbd_close_pipe)},
        {"alloc/free transfer", nl_hassyscall(usbd_alloc_xfer) && nl_hassyscall(usbd_free_xfer)},
        {"setup/submit transfer", nl_hassyscall(usbd_setup_xfer) && nl_hassyscall(usbd_transfer)},
        {"sync transfer", nl_hassyscall(usbd_sync_transfer)},
        {"control request", nl_hassyscall(usbd_do_request)},
        {"set interface", nl_hassyscall(usbd_set_interface)},
        {"driver registration", nl_hassyscall(usb_register_driver)}
    };
    unsigned i, available = 0;
    int parse_ok = cxi_rtl_parse_config(cfg, sizeof(cfg), &profile) == CXI_RTL_CORE_OK;
    int required_ok;

    for (i = 0; i < sizeof(checks)/sizeof(checks[0]); ++i)
        if (checks[i].present) ++available;

    required_ok = checks[0].present && checks[1].present &&
                  checks[2].present && checks[3].present &&
                  checks[5].present && checks[6].present &&
                  checks[7].present && checks[9].present &&
                  checks[10].present && checks[12].present;

    {
        int used = snprintf(report, sizeof(report),
            "RTL8821CU DRIVER CORE 0.4\n"
            "GATE DE COMPATIBILITE USB Ndless\n\n"
            "Profil USB Windows hors ligne: %s\n"
            "Cible: %04X:%04X / interface %u\n"
            "Endpoints attendus: IN 0x84, OUT 0x05/0x06\n\n"
            "API USB presentes: %u/%u\n",
            parse_ok ? "VALIDE" : "INVALIDE",
            (unsigned)CXI_RTL8821CU_VID, (unsigned)CXI_RTL8821CU_PID,
            (unsigned)profile.wifi.number, available,
            (unsigned)(sizeof(checks)/sizeof(checks[0])));
        for (i = 0; i < sizeof(checks)/sizeof(checks[0]) && used < (int)sizeof(report)-80; ++i) {
            used += snprintf(report + used, sizeof(report) - (size_t)used,
                "%s %s\n", checks[i].present ? "[OK]" : "[--]", checks[i].name);
        }
        snprintf(report + used, sizeof(report) - (size_t)used,
            "\nPrerequis API enumeration: %s\n"
            "Aucun appareil ouvert; aucune interface revendiquee.\n"
            "Aucune requete USB, aucun transfert, aucun firmware.\n"
            "Le PID 0BDA:1A2B observe sur la CX reste non associe.\n"
            "Prochaine etape: obtenir un handle USB valide.",
            (parse_ok && required_ok) ? "SATISFAITS" : "INCOMPLETS");
    }
    show_msgbox("RTL8821CU Core 0.4", report);
    return (parse_ok && required_ok) ? 0 : 1;
}
