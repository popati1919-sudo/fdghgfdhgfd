/* RTL8821CU driver core: hardware-independent USB descriptor/state logic.
 * This module performs no USB I/O and is safe to host-test.
 */
#ifndef CXI_RTL8821CU_CORE_H
#define CXI_RTL8821CU_CORE_H
#include <stddef.h>
#include <stdint.h>

#define CXI_RTL8821CU_VID 0x0bdaU
#define CXI_RTL8821CU_PID 0xc820U
#define CXI_RTL8821CU_IFACE_CLASS 0xffU
#define CXI_RTL8821CU_MAX_ENDPOINTS 8U

typedef enum {
    CXI_RTL_CORE_OK = 0,
    CXI_RTL_CORE_BAD_ARGUMENT,
    CXI_RTL_CORE_BAD_CONFIG,
    CXI_RTL_CORE_TRUNCATED,
    CXI_RTL_CORE_TOO_MANY_ENDPOINTS,
    CXI_RTL_CORE_NO_WIFI_INTERFACE,
    CXI_RTL_CORE_BAD_TRANSITION
} CxiRtlCoreStatus;

typedef struct {
    uint8_t address;
    uint8_t attributes;
    uint16_t max_packet;
} CxiRtlEndpoint;

typedef struct {
    uint8_t number;
    uint8_t alternate;
    uint8_t klass;
    uint8_t subclass;
    uint8_t protocol;
    uint8_t declared_endpoints;
    uint8_t endpoint_count;
    CxiRtlEndpoint endpoints[CXI_RTL8821CU_MAX_ENDPOINTS];
} CxiRtlInterface;

typedef struct {
    uint8_t configuration_value;
    uint8_t interface_count;
    CxiRtlInterface wifi;
    uint8_t wifi_found;
    uint8_t bulk_in_count;
    uint8_t bulk_out_count;
} CxiRtlDeviceProfile;

typedef enum {
    CXI_RTL_STAGE_DETACHED = 0,
    CXI_RTL_STAGE_ENUMERATED,
    CXI_RTL_STAGE_INTERFACE_READY,
    CXI_RTL_STAGE_POWERED,
    CXI_RTL_STAGE_FIRMWARE_LOADED,
    CXI_RTL_STAGE_MAC_READY,
    CXI_RTL_STAGE_COMMAND_CHANNEL_READY,
    CXI_RTL_STAGE_SCANNING
} CxiRtlStage;

static inline uint16_t cxi_rtl_le16(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

/* Parses a standard USB configuration descriptor byte stream. */
static inline CxiRtlCoreStatus cxi_rtl_parse_config(
    const uint8_t *buf, size_t len, CxiRtlDeviceProfile *out)
{
    size_t off = 0;
    CxiRtlInterface *current = 0;
    if (!buf || !out) return CXI_RTL_CORE_BAD_ARGUMENT;
    if (len < 9U) return CXI_RTL_CORE_TRUNCATED;
    if (buf[0] < 9U || buf[1] != 2U) return CXI_RTL_CORE_BAD_CONFIG;
    if (cxi_rtl_le16(buf + 2) > len) return CXI_RTL_CORE_TRUNCATED;

    out->configuration_value = buf[5];
    out->interface_count = buf[4];
    out->wifi_found = 0;
    out->bulk_in_count = 0;
    out->bulk_out_count = 0;

    while (off < cxi_rtl_le16(buf + 2)) {
        uint8_t dlen, dtype;
        if (cxi_rtl_le16(buf + 2) - off < 2U) return CXI_RTL_CORE_TRUNCATED;
        dlen = buf[off]; dtype = buf[off + 1U];
        if (dlen < 2U || dlen > cxi_rtl_le16(buf + 2) - off)
            return CXI_RTL_CORE_BAD_CONFIG;
        if (dtype == 4U) {
            if (dlen < 9U) return CXI_RTL_CORE_BAD_CONFIG;
            current = 0;
            if (buf[off + 5U] == CXI_RTL8821CU_IFACE_CLASS) {
                out->wifi.number = buf[off + 2U];
                out->wifi.alternate = buf[off + 3U];
                out->wifi.declared_endpoints = buf[off + 4U];
                out->wifi.klass = buf[off + 5U];
                out->wifi.subclass = buf[off + 6U];
                out->wifi.protocol = buf[off + 7U];
                out->wifi.endpoint_count = 0;
                out->wifi_found = 1;
                current = &out->wifi;
            }
        } else if (dtype == 5U && current) {
            CxiRtlEndpoint *ep;
            if (dlen < 7U) return CXI_RTL_CORE_BAD_CONFIG;
            if (current->endpoint_count >= CXI_RTL8821CU_MAX_ENDPOINTS)
                return CXI_RTL_CORE_TOO_MANY_ENDPOINTS;
            ep = &current->endpoints[current->endpoint_count++];
            ep->address = buf[off + 2U];
            ep->attributes = buf[off + 3U];
            ep->max_packet = cxi_rtl_le16(buf + off + 4U) & 0x07ffU;
            if ((ep->attributes & 3U) == 2U) {
                if (ep->address & 0x80U) out->bulk_in_count++;
                else out->bulk_out_count++;
            }
        }
        off += dlen;
    }
    if (!out->wifi_found) return CXI_RTL_CORE_NO_WIFI_INTERFACE;
    return CXI_RTL_CORE_OK;
}

/* Enforces initialization order; never performs hardware operations. */
static inline CxiRtlCoreStatus cxi_rtl_advance_stage(CxiRtlStage *stage,
                                                     CxiRtlStage next)
{
    if (!stage || next < CXI_RTL_STAGE_ENUMERATED ||
        next > CXI_RTL_STAGE_SCANNING)
        return CXI_RTL_CORE_BAD_ARGUMENT;
    if ((int)next != (int)*stage + 1)
        return CXI_RTL_CORE_BAD_TRANSITION;
    *stage = next;
    return CXI_RTL_CORE_OK;
}
#endif
