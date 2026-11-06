/* RTL8821C rtw88 firmware/protocol facts, extracted for offline validation.
 * No USB operations are implemented here.
 */
#ifndef CXI_RTL8821CU_PROTOCOL_H
#define CXI_RTL8821CU_PROTOCOL_H
#include <stddef.h>
#include <stdint.h>

#define CXI_RTW18_FW_NAME "rtw88/rtw8821c_fw.bin"
#define CXI_RTW18_FW_HEADER_SIZE 64U
#define CXI_RTW18_USB_VENDOR_REQ 0x05U
#define CXI_RTW18_USB_READ_BMREQ 0xc0U
#define CXI_RTW18_USB_WRITE_BMREQ 0x40U
#define CXI_RTW18_USB_REG_INDEX 0x00U
#define CXI_RTW18_EXPECTED_VID 0x0bdaU
#define CXI_RTW18_EXPECTED_PID 0xc820U
#define CXI_RTW18_VENDOR_CLASS 0xffU

typedef enum {
    CXI_RTW18_FW_OK = 0,
    CXI_RTW18_FW_NULL,
    CXI_RTW18_FW_TOO_SHORT
} CxiRtw18FwStatus;

typedef struct {
    uint16_t signature;
    uint16_t version;
    uint8_t subversion;
    uint8_t subindex;
    uint32_t feature;
    uint16_t year;
    uint8_t month, day, hour, minute;
    uint16_t h2c_format_version;
} CxiRtw18FwInfo;

typedef enum {
    CXI_RTW18_STAGE_USB_ENUMERATION = 0,
    CXI_RTW18_STAGE_INTERFACE_PARSE,
    CXI_RTW18_STAGE_CHIP_POWER_ON,
    CXI_RTW18_STAGE_FIRMWARE_DOWNLOAD,
    CXI_RTW18_STAGE_MAC_PHY_CONFIGURATION,
    CXI_RTW18_STAGE_COMMAND_EVENT_CHANNEL,
    CXI_RTW18_STAGE_WIFI_SCAN,
    CXI_RTW18_STAGE_COUNT
} CxiRtw18InitStage;

static inline uint16_t cxi_rtw18_le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static inline uint32_t cxi_rtw18_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* Parses the 64-byte rtw_fw_hdr layout used by Linux rtw88.
 * The parser intentionally does not assert a signature value: firmware
 * revisions/chip variants must be checked against the exact upstream source.
 */
static inline CxiRtw18FwStatus cxi_rtw18_parse_fw_header(
    const uint8_t *data, size_t length, CxiRtw18FwInfo *out)
{
    if (!data || !out) return CXI_RTW18_FW_NULL;
    if (length < CXI_RTW18_FW_HEADER_SIZE) return CXI_RTW18_FW_TOO_SHORT;
    out->signature = cxi_rtw18_le16(data + 0x00);
    out->version = cxi_rtw18_le16(data + 0x04);
    out->subversion = data[0x06];
    out->subindex = data[0x07];
    out->feature = cxi_rtw18_le32(data + 0x0c);
    out->month = data[0x10];
    out->day = data[0x11];
    out->hour = data[0x12];
    out->minute = data[0x13];
    out->year = cxi_rtw18_le16(data + 0x14);
    out->h2c_format_version = cxi_rtw18_le16(data + 0x1c);
    return CXI_RTW18_FW_OK;
}

/* rtw88 USB register access is vendor control traffic, not BOT/SCSI.
 * These are protocol descriptors only; callers must not issue them yet.
 */
static inline int cxi_rtw18_usb_protocol_is_consistent(void)
{
    return CXI_RTW18_USB_VENDOR_REQ == 0x05U &&
           CXI_RTW18_USB_READ_BMREQ == 0xc0U &&
           CXI_RTW18_USB_WRITE_BMREQ == 0x40U &&
           CXI_RTW18_USB_REG_INDEX == 0x00U;
}
#endif
