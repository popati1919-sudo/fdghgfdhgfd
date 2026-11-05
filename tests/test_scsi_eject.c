#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../src/scsi_eject.h"

int main(void) {
    uint8_t p[CXI_CBW_SIZE];
    cxi_make_eject_cbw(p,0x12345678U);
    assert(cxi_validate_eject_cbw(p,sizeof p));
    assert(p[0]=='U' && p[1]=='S' && p[2]=='B' && p[3]=='C');
    assert(p[4]==0x78 && p[5]==0x56 && p[6]==0x34 && p[7]==0x12);
    assert(p[14]==6 && p[15]==0x1b && p[19]==0x02);
    assert(!cxi_validate_eject_cbw(p,sizeof p-1));
    p[12]=0x80; assert(!cxi_validate_eject_cbw(p,sizeof p));
    cxi_make_eject_cbw(p,1); p[19]=0x03;
    assert(!cxi_validate_eject_cbw(p,sizeof p));
    puts("SCSI EJECT CBW tests: OK");
    return 0;
}
