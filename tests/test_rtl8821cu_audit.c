#include <assert.h>
#include <stdio.h>
#include "../src/rtl8821cu_audit.h"

int main(void)
{
    const CxiRtlWindowsProfile good = {
        0x0bdaU, 0xc820U, 2U, 0xffU, 0x84U, {0x05U, 0x06U, 0x08U}
    };
    CxiRtlWindowsProfile bad = good;
    assert(cxi_rtl8821cu_audit_profile(&good) == CXI_AUDIT_OK);
    bad.pid = 0x1a2bU;
    assert(cxi_rtl8821cu_audit_profile(&bad) == CXI_AUDIT_BAD_ID);
    bad = good; bad.interface_class = 0x08U;
    assert(cxi_rtl8821cu_audit_profile(&bad) == CXI_AUDIT_BAD_INTERFACE);
    bad = good; bad.bulk_in = 0x04U;
    assert(cxi_rtl8821cu_audit_profile(&bad) == CXI_AUDIT_BAD_ENDPOINTS);
    bad = good; bad.bulk_out[2] = 0x06U;
    assert(cxi_rtl8821cu_audit_profile(&bad) == CXI_AUDIT_BAD_ENDPOINTS);
    puts("OK: audit RTL8821CU (identite, interface, endpoints) passe");
    return 0;
}
