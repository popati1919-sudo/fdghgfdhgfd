/* RTL8821CU Mode Switch 0.9 - safe SCSI EJECT preparation.
 * Builds and validates a USB Mass Storage Bulk-Only CBW in RAM only.
 * Ndless SDK headers available to this project expose no usbd_open_pipe /
 * usbd_bulk_transfer API, so this version deliberately sends no USB I/O.
 */
#include <os.h>
#include <libndls.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "scsi_eject.h"

static void hexline(char *out, const uint8_t *p, unsigned n) {
    unsigned i; char tmp[4]; out[0]=0;
    for(i=0;i<n;i++){snprintf(tmp,sizeof(tmp),"%02X ",p[i]);strcat(out,tmp);}
}
int main(void) {
    uint8_t cbw[CXI_CBW_SIZE];
    char bytes[128], report[900];
    int valid, choice=0;
    cxi_make_eject_cbw(cbw,0x0900);
    valid=cxi_validate_eject_cbw(cbw,sizeof(cbw));
    hexline(bytes,cbw,sizeof(cbw));
    nl_set_resident();
    do {
        snprintf(report,sizeof(report),
          "RTL8821CU MODE SWITCH 0.9\n"
          "-------------------------\n"
          "Cible : 0BDA:1A2B -> 0BDA:C820\n\n"
          "Commande SCSI : START STOP UNIT\n"
          "LOEJ=1 / START=0\n"
          "CBW BOT : %s\n"
          "Taille : %u octets\n"
          "Validation paquet : %s\n\n"
          "CBW (hex) :\n%s\n"
          "Transport Bulk IN/OUT : INDISPONIBLE\n"
          "Aucun endpoint BOT confirme.\n"
          "AUCUNE COMMANDE N'EST ENVOYEE.\n"
          "Ne constitue pas encore un changement de mode.",
          valid?"PREPARE":"INVALIDE",(unsigned)sizeof(cbw),
          valid?"OK":"ECHEC",bytes);
        show_msgbox("RTL8821CU Mode Switch 0.9",report);
        choice=show_msgbox_2b("MODE SWITCH 0.9",
          "Recalculer et verifier le paquet SCSI EJECT ?",
          "Verifier","Quitter");
    } while(choice==1);
    _exit(0);
}
