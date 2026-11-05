#include <assert.h>
#include <stdio.h>
#include "../src/usb_bot_gate.h"
int main(void) {
 CxiBotProfile c820={0xff,0xff,0xff,0x84,0x05};
 CxiBotProfile bot={0x08,0x06,0x50,0x84,0x05};
 CxiBotProfile no_in={0x08,0x06,0x50,0x04,0x05};
 assert(cxi_bot_check(&c820)==CXI_BOT_NOT_MASS_STORAGE);
 assert(cxi_bot_check(&bot)==CXI_BOT_OK);
 assert(cxi_bot_check(&no_in)==CXI_BOT_BAD_ENDPOINTS);
 assert(cxi_bot_check(0)==CXI_BOT_BAD_ENDPOINTS);
 puts("USB BOT gate tests: OK"); return 0;
}
