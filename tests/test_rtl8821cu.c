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

static void test_windows_endpoint_layout(void)
{
    /* Extrait statique USBTreeView: interface MI_02 et cinq endpoints.
     * Il ne s'agit PAS d'une lecture effectuee par la calculatrice. */
    const uint8_t raw[] = {
        9,2,53,0,1,1,0,0xa0,250,
        9,4,2,0,5,0xff,0xff,0xff,2,
        7,5,0x84,0x02,0x00,0x02,0,
        7,5,0x05,0x02,0x00,0x02,0,
        7,5,0x06,0x02,0x00,0x02,0,
        7,5,0x87,0x03,0x40,0x00,3,
        7,5,0x08,0x02,0x00,0x02,0
    };
    CxiUsbConfiguration cfg;
    CxiRtlInterface iface;
    assert(cxi_usb_parse_configuration(raw, sizeof raw, &cfg) == CXI_USB_DESC_OK);
    assert(cxi_rtl8821cu_select_interface(0x0bda,0xc820,&cfg,&iface)
           == CXI_RTL_INTERFACE_FOUND);
    assert(iface.interface_number == 2);
    assert(iface.bulk_in == 0x84 && iface.bulk_in_max_packet == 512);
    assert(iface.bulk_out_count == 3);
    assert(iface.bulk_out[0] == 0x05);
    assert(iface.bulk_out[1] == 0x06);
    assert(iface.bulk_out[2] == 0x08);
    assert(iface.bulk_out_max_packet[0] == 512);
    assert(iface.interrupt_in == 0x87 && iface.interrupt_max_packet == 64);
    assert(cxi_rtl8821cu_select_interface(0x0bda,0x1a2b,&cfg,&iface)
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
    assert(cxi_usb_parse_configuration(raw,sizeof raw,&cfg) == CXI_USB_DESC_OK);
    assert(cxi_rtl8821cu_select_interface(0x0bda,0xc820,&cfg,&iface)
           == CXI_RTL_INTERFACE_INCOMPLETE);
    assert(cxi_rtl8821cu_select_interface(0x0bda,0xc820,NULL,&iface)
           == CXI_RTL_BAD_CONFIGURATION);
}

int main(void)
{
    test_identity();
    test_windows_endpoint_layout();
    test_incomplete_and_bad();
    puts("OK: RTL8821CU core - identite, 5 endpoints, limites");
    return 0;
}
