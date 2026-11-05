/* CX Internet - Probe 9 safe: fiche USB RTL8821CU.
 * Reference only: values transcribed from the supplied Windows UsbTreeView report.
 * No USB driver registration, pointer access, endpoint transfer or firmware load.
 */
#include <os.h>
#include <libndls.h>
#define TITLE "CX Internet - USB Probe 9"

int main(void)
{
    const char *msg =
        "PROBE 9 - RTL8821CU / USB\n"
        "Source: rapport UsbTreeView Windows\n"
        "VID:PID: 0BDA:C820\n"
        "Interface Wi-Fi: 2 (MI_02)\n"
        "Classe: FF/FF/FF (Realtek)\n\n"
        "ENDPOINTS WINDOWS\n"
        "0x84 IN  - Bulk      - 512 octets\n"
        "0x05 OUT - Bulk      - 512 octets\n"
        "0x06 OUT - Bulk      - 512 octets\n"
        "0x87 IN  - Interrupt - 64 octets\n"
        "0x08 OUT - Bulk      - 512 octets\n\n"
        "ETAT SUR TI-NSPIRE: NON VERIFIE\n"
        "Aucun transfert USB effectue.\n"
        "Aucun pilote RTL8821CU charge.\n"
        "Support des endpoints par la TI-Nspire\n"
        "non demontre.";
    show_msgbox(TITLE, msg);
    return 0;
}
