/* CX Internet - coeur de selection d'interface RTL8821CU.
 * GPL-3.0-or-later. Couche pure: aucune transaction USB ni acces radio.
 * Les donnees viennent d'un descripteur USB deja obtenu par une couche hote.
 */
#ifndef CXI_RTL8821CU_H
#define CXI_RTL8821CU_H
#include <stdint.h>
#include <stddef.h>
#include "usb_descriptors.h"

#define CXI_RTL_VID 0x0bdaU
#define CXI_RTL8821CU_PID 0xc820U
#define CXI_RTL_MAX_BULK_OUT 4U

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
    uint8_t bulk_out[CXI_RTL_MAX_BULK_OUT];
    uint8_t bulk_out_count;
    uint8_t interrupt_in;
    uint16_t bulk_in_max_packet;
    uint16_t bulk_out_max_packet[CXI_RTL_MAX_BULK_OUT];
    uint16_t interrupt_max_packet;
} CxiRtlInterface;

/* 0BDA:1A2B n'est pas accepte: seule l'identite finale 0BDA:C820
 * est associee au RTL8821CU. Le mode 1A2B doit etre traite separement. */
static inline int cxi_rtl8821cu_id_matches(uint16_t vid, uint16_t pid)
{
    return vid == CXI_RTL_VID && pid == CXI_RTL8821CU_PID;
}

/* Selectionne l'interface vendor-specific avec Bulk IN, au moins un Bulk
 * OUT, et memorise aussi tous les Bulk OUT et Interrupt IN disponibles.
 * Aucun endpoint n'est contacte ici. */
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
        CxiRtlInterface c = {0};
        if (it->klass != 0xffU) continue;
        saw_vendor = 1;
        c.interface_number = it->number;
        for (j = 0; j < cfg->endpoint_count; ++j) {
            const CxiUsbEndpoint *ep = &cfg->endpoints[j];
            unsigned transfer = ep->attributes & 3U;
            if (ep->interface_number != it->number || !ep->max_packet)
                continue;
            if (transfer == 2U && (ep->address & 0x80U)) {
                if (!c.bulk_in) {
                    c.bulk_in = ep->address;
                    c.bulk_in_max_packet = ep->max_packet & 0x07ffU;
                }
            } else if (transfer == 2U && !(ep->address & 0x80U)) {
                if (c.bulk_out_count < CXI_RTL_MAX_BULK_OUT) {
                    unsigned k = c.bulk_out_count++;
                    c.bulk_out[k] = ep->address;
                    c.bulk_out_max_packet[k] = ep->max_packet & 0x07ffU;
                }
            } else if (transfer == 3U && (ep->address & 0x80U) &&
                       !c.interrupt_in) {
                c.interrupt_in = ep->address;
                c.interrupt_max_packet = ep->max_packet & 0x07ffU;
            }
        }
        if (c.bulk_in && c.bulk_out_count) {
            *selected = c;
            return CXI_RTL_INTERFACE_FOUND;
        }
    }
    return saw_vendor ? CXI_RTL_INTERFACE_INCOMPLETE : CXI_RTL_BAD_CONFIGURATION;
}
#endif
