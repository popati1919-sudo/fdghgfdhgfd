#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/rtl8821cu.h"

static void test_identity(void)
{
    assert(cxi_rtl8821cu_id_matches(0x0bda, 0xc820));
    assert(!cxi_rtl8821cu_id_matches(0x0bda, 0x1a2b));
    assert(!cxi_rtl8821cu_id_matches(0x1234, 0xc820));
}

static void test_interface(void)
{
    /* Configuration, vendor-specific interface 2, bulk OUT 0x02 + IN 0x81. */
    const uint8_t raw[] = {
        9,2,32,0,1,1,0,0x80,50,
        9,4,2,0,2,0xff,0xff,0xff,0,
        7,5,0x02,0x02,0x00,0x02,0,
        7,5,0x81,0x02,0x00,0x02,0
    };
    CxiUsbConfiguration cfg;
    CxiRtlInterface iface;
    assert(cxi_usb_parse_configuration(raw, sizeof raw, &cfg) == CXI_USB_DESC_OK);
    assert(cxi_rtl8821cu_select_interface(0x0bda, 0xc820, &cfg, &iface)
           == CXI_RTL_INTERFACE_FOUND);
    assert(iface.interface_number == 2);
    assert(iface.bulk_out == 0x02 && iface.bulk_in == 0x81);
    assert(iface.bulk_in_max_packet == 512 && iface.bulk_out_max_packet == 512);
    assert(cxi_rtl8821cu_select_interface(0x0bda, 0x1a2b, &cfg, &iface)
           == CXI_RTL_NOT_MATCHED);
}

static void test_incomplete_and_bad(void)
{
    const uint8_t raw[] = {
        9,2,25,0,1,1,0,0x80,50,
        9,4,0,0,1,0xff,0,0,0,
        7,5,0x81,0x02,0x00,0x02,0
    };
    CxiUsbConfiguration cfg;
    CxiRtlInterface iface;
    assert(cxi_usb_parse_configuration(raw, sizeof raw, &cfg) == CXI_USB_DESC_OK);
    assert(cxi_rtl8821cu_select_interface(0x0bda, 0xc820, &cfg, &iface)
           == CXI_RTL_INTERFACE_INCOMPLETE);
    assert(cxi_rtl8821cu_select_interface(0x0bda, 0xc820, NULL, &iface)
           == CXI_RTL_BAD_CONFIGURATION);
}

int main(void)
{
    test_identity();
    test_interface();
    test_incomplete_and_bad();
    puts("OK: tests RTL8821CU (identite + endpoints) passes");
    return 0;
}
