#include <assert.h>
#include <stdio.h>
#include "../src/rtl8821cu_core.h"

int main(void) {
    /* Minimal synthetic configuration with MI_02 and three bulk endpoints. */
    const uint8_t cfg[] = {
      9,2, 39,0, 1,1,0,0x80,50,
      9,4, 2,0,3,0xff,0xff,0xff,0,
      7,5,0x84,2,0,2,0,
      7,5,0x05,2,0,2,0,
      7,5,0x06,2,0,2,0
    };
    CxiRtlDeviceProfile p;
    CxiRtlStage s = CXI_RTL_STAGE_DETACHED;
    assert(cxi_rtl_parse_config(cfg, sizeof cfg, &p) == CXI_RTL_CORE_OK);
    assert(p.wifi_found && p.wifi.number == 2 && p.wifi.klass == 0xff);
    assert(p.wifi.endpoint_count == 3);
    assert(p.bulk_in_count == 1 && p.bulk_out_count == 2);
    assert(cxi_rtl_advance_stage(&s, CXI_RTL_STAGE_ENUMERATED) == CXI_RTL_CORE_OK);
    assert(cxi_rtl_advance_stage(&s, CXI_RTL_STAGE_POWERED) == CXI_RTL_CORE_BAD_TRANSITION);
    assert(cxi_rtl_advance_stage(&s, CXI_RTL_STAGE_INTERFACE_READY) == CXI_RTL_CORE_OK);
    assert(cxi_rtl_parse_config(cfg, 5, &p) == CXI_RTL_CORE_TRUNCATED);
    puts("RTL8821CU core: descriptor parsing and stage-gate tests OK");
    return 0;
}
