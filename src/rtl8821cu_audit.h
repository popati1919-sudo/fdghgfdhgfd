/* Audit hors-ligne du profil Windows RTL8821CU.
 * Donnees recopiees du rapport USBTreeView et de netrtwlanu.inf.
 * Aucun acces USB, aucun chargement du binaire Windows.
 */
#ifndef CXI_RTL8821CU_AUDIT_H
#define CXI_RTL8821CU_AUDIT_H
#include <stdint.h>
#define CXI_AUDIT_VID 0x0bdaU
#define CXI_AUDIT_PID 0xc820U
#define CXI_AUDIT_MI  0x02U
#define CXI_AUDIT_CLASS 0xffU
#define CXI_AUDIT_BULK_IN 0x84U
#define CXI_AUDIT_BULK_OUT_COUNT 3U

typedef struct {
    uint16_t vid, pid;
    uint8_t interface_number, interface_class;
    uint8_t bulk_in, bulk_out[CXI_AUDIT_BULK_OUT_COUNT];
} CxiRtlWindowsProfile;

typedef enum {
    CXI_AUDIT_OK = 0,
    CXI_AUDIT_BAD_ID,
    CXI_AUDIT_BAD_INTERFACE,
    CXI_AUDIT_BAD_ENDPOINTS
} CxiRtlAuditResult;

static inline int cxi_audit_is_bulk_out(uint8_t ep)
{
    return ep != 0U && (ep & 0x80U) == 0U && (ep & 0x0fU) != 0U;
}

static inline CxiRtlAuditResult cxi_rtl8821cu_audit_profile(
    const CxiRtlWindowsProfile *p)
{
    unsigned i;
    if (!p || p->vid != CXI_AUDIT_VID || p->pid != CXI_AUDIT_PID)
        return CXI_AUDIT_BAD_ID;
    if (p->interface_number != CXI_AUDIT_MI ||
        p->interface_class != CXI_AUDIT_CLASS)
        return CXI_AUDIT_BAD_INTERFACE;
    if (p->bulk_in != CXI_AUDIT_BULK_IN ||
        (p->bulk_in & 0x80U) == 0U || (p->bulk_in & 0x0fU) == 0U)
        return CXI_AUDIT_BAD_ENDPOINTS;
    for (i = 0; i < CXI_AUDIT_BULK_OUT_COUNT; ++i) {
        unsigned j;
        if (!cxi_audit_is_bulk_out(p->bulk_out[i])) return CXI_AUDIT_BAD_ENDPOINTS;
        for (j = 0; j < i; ++j)
            if (p->bulk_out[i] == p->bulk_out[j]) return CXI_AUDIT_BAD_ENDPOINTS;
    }
    return CXI_AUDIT_OK;
}
#endif
