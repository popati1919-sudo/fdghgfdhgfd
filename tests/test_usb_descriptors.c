#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/usb_descriptors.h"

int main(void)
{
    /* Configuration fictive de test: interface vendor-specific et deux bulk. */
    const uint8_t good[] = {
        9,2,32,0,1,1,0,0x80,250,
        9,4,0,0,2,0xff,0xff,0xff,0,
        7,5,0x01,2,0x00,0x02,0,
        7,5,0x81,2,0x00,0x02,0
    };
    CxiUsbConfiguration c;
    assert(cxi_usb_parse_configuration(good, sizeof good, &c) == CXI_USB_DESC_OK);
    assert(c.total_length == 32 && c.configuration_value == 1);
    assert(c.interface_count == 1 && c.endpoint_count == 2);
    assert(c.interfaces[0].klass == 0xff && c.interfaces[0].endpoint_count == 2);
    assert(c.endpoints[0].address == 0x01 && c.endpoints[0].max_packet == 512);
    assert(c.endpoints[1].address == 0x81 && c.endpoints[1].interface_number == 0);

    assert(cxi_usb_parse_configuration(NULL, 0, &c) == CXI_USB_DESC_NULL);
    assert(cxi_usb_parse_configuration(good, 8, &c) == CXI_USB_DESC_SHORT);
    {
        uint8_t bad[sizeof good];
        memcpy(bad, good, sizeof good);
        bad[2] = 0xff; bad[3] = 0x7f;
        assert(cxi_usb_parse_configuration(bad, sizeof bad, &c) == CXI_USB_DESC_BAD_LENGTH);
        memcpy(bad, good, sizeof good);
        bad[10] = 1; /* descriptor interface tronque */
        assert(cxi_usb_parse_configuration(bad, sizeof bad, &c) == CXI_USB_DESC_BAD_LENGTH);
    }
    puts("OK: analyseur USB (configuration/interfaces/endpoints)");
    return 0;
}
