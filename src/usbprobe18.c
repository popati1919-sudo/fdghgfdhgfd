/* CX Internet - Probe 18: audit protocole/firmware RTL8821C.
 * Offline only: no USB enumeration, control request, bulk transfer or scan.
 */
#include <os.h>
#include <libndls.h>
#include <stdio.h>
#include "rtl8821cu_protocol.h"

int main(void)
{
    char report[1024];
    snprintf(report, sizeof report,
        "PROBE 18 - PROTOCOLE RTL8821CU\n\n"
        "Firmware WLAN attendu:\n%s\n"
        "Entete rtw88: %u octets (parseur C ajoute)\n"
        "Champs: version, sous-version, date, feature, H2C\n\n"
        "USB Realtek vendor control (reference Linux):\n"
        "bRequest: 0x%02X\n"
        "Lecture: bmRequestType 0x%02X\n"
        "Ecriture: bmRequestType 0x%02X\n"
        "wIndex: 0x%02X (registre via wValue)\n"
        "BOT/SCSI: non utilise pour l'interface Wi-Fi\n\n"
        "Ordre requis du pilote:\n"
        "1. Enumeration + analyse interface/endpoints\n"
        "2. Mise sous tension du chipset\n"
        "3. Chargement firmware WLAN\n"
        "4. Configuration MAC/PHY + calibration\n"
        "5. Canal commandes/evenements H2C/C2H\n"
        "6. Demande de scan + reception resultats\n\n"
        "Controle protocole: %s\n"
        "API USB Ndless: non validee pour ce peripherique\n"
        "AUCUNE requete USB envoyee. AUCUNE ecriture registre.\n"
        "Aucun firmware charge; aucun scan lance.",
        CXI_RTW18_FW_NAME, (unsigned)CXI_RTW18_FW_HEADER_SIZE,
        (unsigned)CXI_RTW18_USB_VENDOR_REQ,
        (unsigned)CXI_RTW18_USB_READ_BMREQ,
        (unsigned)CXI_RTW18_USB_WRITE_BMREQ,
        (unsigned)CXI_RTW18_USB_REG_INDEX,
        cxi_rtw18_usb_protocol_is_consistent() ? "OK" : "ECHEC");
    show_msgbox("USB Probe 18", report);
    return cxi_rtw18_usb_protocol_is_consistent() ? 0 : 1;
}
