/* CX Internet - Probe 15: validation logicielle SCSI/BOT uniquement. */
#include <os.h>
#include <libndls.h>
#include <stdint.h>
#include <stdio.h>
#include "scsi_eject.h"

int main(void) {
    uint8_t cbw[CXI_CBW_SIZE];
    char report[512];
    cxi_make_eject_cbw(cbw,0x1531U);
    if (!cxi_validate_eject_cbw(cbw,sizeof cbw)) {
        show_msgbox("USB Probe 15","ECHEC: le CBW SCSI EJECT construit est invalide.");
        return 1;
    }
    snprintf(report,sizeof report,
        "PROBE 15 - VALIDATION SCSI EJECT\n\n"
        "CBW: %u octets\nSignature: USBC (0x43425355)\n"
        "Tag: 0x1531\nTransfert de donnees: 0 octet\n"
        "Direction: OUT / aucune donnee\nLUN: 0\n"
        "Longueur CDB: 6\nCommande: START STOP UNIT (0x1B)\n"
        "LOEJ=1, START=0 (0x02)\n\n"
        "Validation logicielle: OK\n\n"
        "AUCUN transfert USB effectue.\n"
        "Aucune commande envoyee a l'antenne.\n"
        "Cela ne confirme pas que le mode stockage BOT est actif.",
        (unsigned)sizeof cbw);
    show_msgbox("USB Probe 15",report);
    return 0;
}
