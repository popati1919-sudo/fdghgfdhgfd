#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/rtl8821cu_protocol.h"

static void test_fw_header(void)
{
    uint8_t raw[CXI_RTW18_FW_HEADER_SIZE];
    CxiRtw18FwInfo info;
    memset(raw, 0, sizeof raw);
    raw[0x00] = 0x34; raw[0x01] = 0x12;
    raw[0x04] = 0x78; raw[0x05] = 0x56;
    raw[0x06] = 0x09; raw[0x07] = 0x03;
    raw[0x0c] = 0xef; raw[0x0d] = 0xcd;
    raw[0x0e] = 0xab; raw[0x0f] = 0x89;
    raw[0x10] = 11; raw[0x11] = 5; raw[0x12] = 13; raw[0x13] = 42;
    raw[0x14] = 0xea; raw[0x15] = 0x07;
    raw[0x1c] = 0x02; raw[0x1d] = 0x00;
    assert(cxi_rtw18_parse_fw_header(raw, sizeof raw, &info) == CXI_RTW18_FW_OK);
    assert(info.signature == 0x1234);
    assert(info.version == 0x5678 && info.subversion == 9 && info.subindex == 3);
    assert(info.feature == 0x89abcdefU);
    assert(info.month == 11 && info.day == 5 && info.hour == 13 && info.minute == 42);
    assert(info.year == 2026 && info.h2c_format_version == 2);
    assert(cxi_rtw18_parse_fw_header(raw, 63, &info) == CXI_RTW18_FW_TOO_SHORT);
    assert(cxi_rtw18_parse_fw_header(NULL, sizeof raw, &info) == CXI_RTW18_FW_NULL);
}
static void test_protocol(void)
{
    assert(strcmp(CXI_RTW18_FW_NAME, "rtw88/rtw8821c_fw.bin") == 0);
    assert(CXI_RTW18_USB_VENDOR_REQ == 0x05);
    assert(CXI_RTW18_USB_READ_BMREQ == 0xc0);
    assert(CXI_RTW18_USB_WRITE_BMREQ == 0x40);
    assert(cxi_rtw18_usb_protocol_is_consistent());
    assert(CXI_RTW18_STAGE_COUNT == 7);
}
int main(void)
{
    test_fw_header();
    test_protocol();
    puts("OK: RTL8821C firmware header + USB protocol audit");
    return 0;
}
