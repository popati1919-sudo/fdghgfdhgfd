/* Probe 16: garde-fou de compatibilite USB Mass Storage BOT.
 * Analyse des champs de descripteur deja connus; aucune E/S USB.
 */
#ifndef CXI_BOT_GATE_H
#define CXI_BOT_GATE_H
#include <stdint.h>
typedef struct {
    uint8_t interface_class, interface_subclass, interface_protocol;
    uint8_t bulk_in, bulk_out;
} CxiBotProfile;
typedef enum {
    CXI_BOT_OK = 0,
    CXI_BOT_NOT_MASS_STORAGE,
    CXI_BOT_NOT_SCSI,
    CXI_BOT_NOT_BULK_ONLY,
    CXI_BOT_BAD_ENDPOINTS
} CxiBotVerdict;
static inline CxiBotVerdict cxi_bot_check(const CxiBotProfile *p) {
    if (!p) return CXI_BOT_BAD_ENDPOINTS;
    if (p->interface_class != 0x08U) return CXI_BOT_NOT_MASS_STORAGE;
    if (p->interface_subclass != 0x06U) return CXI_BOT_NOT_SCSI;
    if (p->interface_protocol != 0x50U) return CXI_BOT_NOT_BULK_ONLY;
    if ((p->bulk_in & 0x80U) == 0U || (p->bulk_in & 0x0fU) == 0U ||
        (p->bulk_out & 0x80U) != 0U || (p->bulk_out & 0x0fU) == 0U)
        return CXI_BOT_BAD_ENDPOINTS;
    return CXI_BOT_OK;
}
#endif
