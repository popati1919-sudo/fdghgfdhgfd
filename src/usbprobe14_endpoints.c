/* CX Internet - Probe 14: audit de l'interface Wi-Fi depuis UsbTreeView.
 * Donnees de reference extraites du rapport Windows, pas lues sur la TI.
 * Analyse logicielle seulement: aucun acces USB ni pilote enregistre.
 */
#include <os.h>
#include <libndls.h>
#include <stdio.h>
#include <string.h>
#include "usb_descriptors.h"
#include "rtl8821cu.h"

#define TITLE "CX Internet - USB Probe 14"

/* Configuration partielle: interface 2 uniquement, exactement comme
 * decrite dans le rapport USBTreeView. Les interfaces Bluetooth 0/1
 * ne sont pas incluses dans cet extrait. */
static const uint8_t wifi_interface_desc[] = {
    9, 2, 53, 0, 1, 1, 0, 0xA0, 250,
    9, 4, 2, 0, 5, 0xFF, 0xFF, 0xFF, 2,
    7, 5, 0x84, 0x02, 0x00, 0x02, 0,
    7, 5, 0x05, 0x02, 0x00, 0x02, 0,
    7, 5, 0x06, 0x02, 0x00, 0x02, 0,
    7, 5, 0x87, 0x03, 0x40, 0x00, 3,
    7, 5, 0x08, 0x02, 0x00, 0x02, 0
};

static const char *ep_type(unsigned a)
{
    switch (a & 3U) {
    case 0: return "Control";
    case 1: return "Iso";
    case 2: return "Bulk";
    default: return "Interrupt";
    }
}

int main(void)
{
    CxiUsbConfiguration cfg;
    CxiRtlInterface selected;
    CxiUsbDescResult parsed;
    CxiRtlResult matched;
    char report[1100];
    char *p = report;
    size_t left = sizeof(report);
    unsigned i;

    memset(&cfg, 0, sizeof(cfg));
    memset(&selected, 0, sizeof(selected));
    parsed = cxi_usb_parse_configuration(wifi_interface_desc,
                                         sizeof(wifi_interface_desc), &cfg);
    matched = cxi_rtl8821cu_select_interface(0x0BDA, 0xC820, &cfg, &selected);

    p += snprintf(p, left,
        "PROBE 14 - AUDIT INTERFACE WIFI\n"
        "Source: rapport USBTreeView Windows\n\n"
        "Cible: Realtek 0BDA:C820\n"
        "Configuration complete: 229 octets / 3 interfaces\n"
        "Extrait teste: interface 2 / 5 endpoints\n"
        "Classe: FF (Vendor Specific)\n"
        "Analyse du descripteur: %s\n"
        "Selection RTL8821CU: %s\n\n"
        "ENDPOINTS INTERFACE 2:\n",
        parsed == CXI_USB_DESC_OK ? "OK" : "ECHEC",
        matched == CXI_RTL_INTERFACE_FOUND ? "OK" : "ECHEC");
    left = sizeof(report) - (size_t)(p - report);

    if (parsed == CXI_USB_DESC_OK) {
        for (i = 0; i < cfg.endpoint_count && left > 80; ++i) {
            const CxiUsbEndpoint *ep = &cfg.endpoints[i];
            p += snprintf(p, left, "0x%02X %-9s %s %u octets\n",
                ep->address, ep_type(ep->attributes),
                (ep->address & 0x80U) ? "IN" : "OUT",
                (unsigned)ep->max_packet);
            left = sizeof(report) - (size_t)(p - report);
        }
    }
    if (left > 100) {
        p += snprintf(p, left,
            "\nCanaux bulk retenus par le selecteur:\n"
            "IN=0x%02X OUT=0x%02X\n"
            "Autres sorties bulk: 0x06 et 0x08\n\n"
            "ATTENTION: extrait statique Windows.\n"
            "Aucun descripteur lu sur la calculatrice.\n"
            "Aucun pilote enregistre, aucun transfert, aucun scan.",
            selected.bulk_in, selected.bulk_out);
    }

    show_msgbox(TITLE, report);
    return (parsed == CXI_USB_DESC_OK &&
            matched == CXI_RTL_INTERFACE_FOUND) ? 0 : 1;
}
