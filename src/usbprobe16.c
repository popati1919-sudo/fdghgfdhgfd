/* CX Internet - Probe 16: verifie si le profil Windows C820 est BOT.
 * Profil issu du rapport USBTreeView fourni; aucune transaction USB.
 */
#include <os.h>
#include <libndls.h>
#include <stdio.h>
#include "usb_bot_gate.h"

int main(void) {
    /* USBTreeView: interface RTL8821CU classe FF (vendor-specific).
       Les endpoints bulk ne transforment pas une interface FF en BOT. */
    const CxiBotProfile c820 = { 0xffU, 0xffU, 0xffU, 0x84U, 0x05U };
    CxiBotVerdict v = cxi_bot_check(&c820);
    char report[512];
    snprintf(report, sizeof report,
        "PROBE 16 - GARDE BOT / SCSI\n\n"
        "Profil Windows importe: 0BDA:C820\n"
        "Interface RTL8821CU: classe FF (proprietaire)\n"
        "Bulk IN: 0x84\nBulk OUT: 0x05 / 0x06 / 0x08\n\n"
        "Controle BOT: %s\n\n"
        "EJECT SCSI: BLOQUE pour C820.\n"
        "Le protocole BOT exige classe 08, sous-classe 06, protocole 50.\n"
        "Il faut auditer le mode 0BDA:1A2B (CD-ROM) separement.\n\n"
        "AUCUN transfert USB effectue.\n"
        "Aucune commande envoyee a l'antenne.",
        v == CXI_BOT_OK ? "COMPATIBLE" : "NON COMPATIBLE - classe USB FF");
    show_msgbox("USB Probe 16", report);
    return 0;
}
