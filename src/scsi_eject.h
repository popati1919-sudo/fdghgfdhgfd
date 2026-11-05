/* Probe 15: constructeur/verificateur BOT SCSI EJECT, sans E/S USB. */
#ifndef CXI_SCSI_EJECT_H
#define CXI_SCSI_EJECT_H
#include <stddef.h>
#include <stdint.h>

#define CXI_CBW_SIZE 31U
#define CXI_CBW_SIGNATURE 0x43425355U

static inline void cxi_put_le32(uint8_t *p, uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}

/* Produit un CBW de test pour START STOP UNIT / LOEJ=1 / START=0.
 * Aucune fonction USB n'est appelee par ce constructeur. */
static inline void cxi_make_eject_cbw(uint8_t cbw[CXI_CBW_SIZE], uint32_t tag) {
    unsigned i;
    for (i=0;i<CXI_CBW_SIZE;i++) cbw[i]=0;
    cxi_put_le32(cbw,CXI_CBW_SIGNATURE);
    cxi_put_le32(cbw+4,tag);
    cbw[14]=6;       /* longueur CDB */
    cbw[15]=0x1b;    /* START STOP UNIT */
    cbw[19]=0x02;    /* LOEJ=1, START=0 */
}

/* Verification stricte du paquet genere; ne valide pas la compatibilite
 * reelle du peripherique ni la disponibilite d'un transport BOT. */
static inline int cxi_validate_eject_cbw(const uint8_t *p, size_t n) {
    unsigned i;
    if (!p || n!=CXI_CBW_SIZE) return 0;
    if (p[0]!='U'||p[1]!='S'||p[2]!='B'||p[3]!='C') return 0;
    if (p[8]!=0||p[9]!=0||p[10]!=0||p[11]!=0) return 0; /* transfert nul */
    if (p[12]!=0) return 0; /* flags: OUT, aucune donnee */
    if (p[13]!=0) return 0; /* LUN 0 */
    if (p[14]!=6) return 0; /* CDB 6 octets */
    if (p[15]!=0x1b || p[19]!=0x02) return 0;
    for (i=16;i<19;i++) if (p[i]!=0) return 0;
    for (i=20;i<CXI_CBW_SIZE;i++) if (p[i]!=0) return 0;
    return 1;
}
#endif
