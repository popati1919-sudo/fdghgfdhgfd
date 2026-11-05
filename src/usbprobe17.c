/* CX Internet - Probe 17: audit hors-ligne RTL8821CU.
 * Les valeurs viennent du rapport USBTreeView Windows et du INF du depot pilot.
 * Aucun transfert USB, aucune enumeration et aucun dereferencement de pointeur.
 */
#include <os.h>
#include <libndls.h>
#include <stdio.h>
#include "rtl8821cu_audit.h"

int main(void)
{
    const CxiRtlWindowsProfile profile = {
        CXI_AUDIT_VID, CXI_AUDIT_PID, CXI_AUDIT_MI, CXI_AUDIT_CLASS,
        CXI_AUDIT_BULK_IN, { 0x05U, 0x06U, 0x08U }
    };
    CxiRtlAuditResult result = cxi_rtl8821cu_audit_profile(&profile);
    char report[768];

    snprintf(report, sizeof report,
        "PROBE 17 - AUDIT RTL8821CU\n\n"
        "Source: Windows USBTreeView + pilot/netrtwlanu.inf\n"
        "INF: 0BDA:C820&MI_02 -> RTL8821cu.ndi\n"
        "Pilote Windows: rtwlanu.sys\n\n"
        "Profil audite: %s\n"
        "VID:PID: 0BDA:C820\n"
        "Interface: MI_02 / classe FF\n"
        "Bulk IN: 0x84\n"
        "Bulk OUT: 0x05, 0x06, 0x08\n\n"
        "Classe proprietaire Realtek: confirmee\n"
        "BOT/SCSI: non applicable a cette interface\n\n"
        "IMPORTANT: profil importe de Windows; pas relu sur la CX.\n"
        "AUCUN transfert USB effectue.\n"
        "Aucun firmware charge; aucun scan Wi-Fi lance.\n\n"
        "Suite: identifier le firmware et les registres RTL8821C,\n"
        "puis verifier les API USB Ndless avant toute E/S.",
        result == CXI_AUDIT_OK ? "OK" : "ECHEC");
    show_msgbox("USB Probe 17", report);
    return result == CXI_AUDIT_OK ? 0 : 1;
}
