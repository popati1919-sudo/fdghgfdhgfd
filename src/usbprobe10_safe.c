/* CX Internet - Probe 10: inventaire passif des API USB Ndless.
 * Ne s'enregistre pas comme pilote, ne lit aucun device_t/ivars,
 * ne suit aucun pointeur et n'effectue aucune transaction USB.
 * GPL-3.0-or-later.
 */
#include <os.h>
#include <libndls.h>
#include <usbdi.h>
#include <syscall-decls.h>
#include <stdint.h>
#include <stdio.h>

#define TITLE "CX Internet - USB Probe 10"

static const char *yesno(int value) { return value ? "DISPONIBLE" : "ABSENTE"; }

int main(void)
{
    char report[640];
    int has_register = nl_hassyscall(usb_register_driver);
    int has_ivars = nl_hassyscall(device_get_ivars);
    int has_malloc = nl_hassyscall(malloc);

    snprintf(report, sizeof(report),
        "PROBE 10 - API USB Ndless\n"
        "Inventaire des fonctions systeme\n\n"
        "usb_register_driver: %s\n"
        "device_get_ivars:    %s\n"
        "malloc:              %s\n\n"
        "RTL8821CU attendu: 0BDA:C820\n"
        "Interface Windows: MI_02\n"
        "Endpoints Windows: 5 (reference seulement)\n\n"
        "AUCUNE ACTION MATERIELLE\n"
        "- aucun pilote enregistre\n"
        "- aucun device_t lu\n"
        "- aucun pointeur suivi\n"
        "- aucun descripteur demande\n"
        "- aucun transfert USB\n"
        "- aucun firmware ou scan Wi-Fi\n\n"
        "Cette sonde verifie uniquement la presence\n"
        "des entrees syscall Ndless indiquees ci-dessus.",
        yesno(has_register), yesno(has_ivars), yesno(has_malloc));

    show_msgbox(TITLE, report);
    return 0;
}
