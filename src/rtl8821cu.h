/* CX Internet - coeur initial du pilote Realtek RTL8821CU.
 * GPL-3.0-or-later. Couche pure: aucun acces materiel ni transaction USB.
 * Elle valide l'identite et les endpoints a partir d'un descripteur deja lu.
 */
#ifndef CXI_RTL8821CU_H
#define CXI_RTL8821CU_H
#include <stdint.h>
#include <stddef.h>
#include "usb_descriptors.h"

#define CXI_RTL_VID 0x0bdaU
#define CXI_RTL8821CU_PID 0xc820U

typedef enum {
    CXI_RTL_NOT_MATCHED = 0,
    CXI_RTL_MATCHED_NEEDS_ENUMERATION,
    CXI_RTL_BAD_CONFIGURATION,
    CXI_RTL_INTERFACE_FOUND,
    CXI_RTL_INTERFACE_INCOMPLETE
} CxiRtlResult;

typedef struct {
    uint8_t interface_number;
    uint8_t bulk_in;
    uint8_t bulk_out;
    uint8_t interrupt_in;
    uint16_t bulk_in_max_packet;
    uint16_t bulk_out_max_packet;
} CxiRtlInterface;

/* Le PID 1A2B observe sur la CX n'est PAS accepte comme RTL8821CU operationnel:
 * il peut correspondre au mode initial/chargeur. On attend le PID C820 confirme
 * sur Windows avant d'associer le pilote radio. */
static inline int cxi_rtl8821cu_id_matches(uint16_t vid, uint16_t pid)
{
    return vid == CXI_RTL_VID && pid == CXI_RTL8821CU_PID;
}

/* Inspection sans I/O: exige une interface vendor-specific et les deux
 * canaux bulk IN/OUT. Le numero d'interface est retourne, jamais devine. */
static inline CxiRtlResult cxi_rtl8821cu_select_interface(
    uint16_t vid, uint16_t pid, const CxiUsbConfiguration *cfg,
    CxiRtlInterface *selected)
{
    unsigned i, j;
    int saw_vendor = 0;
    if (!cxi_rtl8821cu_id_matches(vid, pid)) return CXI_RTL_NOT_MATCHED;
    if (!cfg || !selected || cfg->interface_count > CXI_USB_MAX_INTERFACES ||
        cfg->endpoint_count > CXI_USB_MAX_ENDPOINTS)
        return CXI_RTL_BAD_CONFIGURATION;

    for (i = 0; i < cfg->interface_count; ++i) {
        const CxiUsbInterface *it = &cfg->interfaces[i];
        CxiRtlInterface candidate = {0, 0, 0, 0, 0, 0};
        if (it->klass != 0xffU) continue;
        saw_vendor = 1;
        candidate.interface_number = it->number;
        for (j = 0; j < cfg->endpoint_count; ++j) {
            const CxiUsbEndpoint *ep = &cfg->endpoints[j];
            unsigned transfer = ep->attributes & 3U;
            if (ep->interface_number != it->number || transfer != 2U ||
                ep->max_packet == 0U) continue;
            if ((ep->address & 0x80U) != 0U) {
                if (!candidate.bulk_in) {
                    candidate.bulk_in = ep->address;
                    candidate.bulk_in_max_packet = ep->max_packet;
                }
            } else if (!candidate.bulk_out) {
                candidate.bulk_out = ep->address;
                candidate.bulk_out_max_packet = ep->max_packet;
            }
        }
        if (candidate.bulk_in && candidate.bulk_out) {
            *selected = candidate;
            return CXI_RTL_INTERFACE_FOUND;
        }
    }
    return saw_vendor ? CXI_RTL_INTERFACE_INCOMPLETE : CXI_RTL_BAD_CONFIGURATION;
}
#endif
