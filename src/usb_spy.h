/* CX Internet - etape 1b: structures partagees et decodage (testable sur PC).
 * Licence GPLv3 (les registres et la methode viennent de
 * Nspire-MediaPlayer-Ndless, GPLv3). */
#ifndef CXI_USB_SPY_H
#define CXI_USB_SPY_H
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

#define SPY_RING 4U
#define SPY_ENTRY_WORDS 12U

/* Bloc resident: contexte puis tableau de methodes puis nom du pilote. */
typedef struct {
    uint32_t head[16];                       /* [0]=compteur [1]=decalage ivars */
    uint32_t ring[SPY_RING][SPY_ENTRY_WORDS];
    void *methods[8];
    char name[16];
} SpyResident;

typedef char SpyHeadOffset[(offsetof(SpyResident, ring) == 64) ? 1 : -1];
typedef char SpyEntrySize[(sizeof(((SpyResident *)0)->ring[0]) == 48) ? 1 : -1];

/* Entree: [0]=device_t [1]=usb_attach_arg* [2..9]=copie de usb_attach_arg
 * (port, configno, ifaceno, vendor, product, release, device, iface)
 * [10]=mot a device+8 (adresse|config<<8|profondeur<<16|vitesse<<24)
 * [11]=etape atteinte (0..4; 4 = tout lu) */
enum { E_DEV = 0, E_ARG = 1, E_PORT = 2, E_CONF = 3, E_IFACE = 4, E_VID = 5,
       E_PID = 6, E_REL = 7, E_DEVICE = 8, E_IFH = 9, E_DEVWORD = 10, E_STAGE = 11 };

/* Meme controle que getter() du lecteur video: la fonction exportee doit etre
 * "ldr r0,[r0,#off]" puis "bx lr" (ou "mov pc,lr"). */
static inline int spy_decode_getter(uint32_t w0, uint32_t w1, unsigned *offset)
{
    if ((w0 & 0xfffff000U) != 0xe5900000U || (w0 & 3U) || (w0 & 0xfffU) > 64U ||
        (w1 != 0xe12fff1eU && w1 != 0xe1a0f00eU))
        return 0;
    *offset = w0 & 0xfffU;
    return 1;
}

static inline const char *spy_speed_name(unsigned s)
{
    /* Valeurs NetBSD: 1 bas debit, 2 plein debit, 3 haut debit */
    return s == 1 ? "bas" : s == 2 ? "plein" : s == 3 ? "haut" : "?";
}

static inline const char *spy_vendor_name(unsigned vid)
{
    return vid == 0x0bdaU ? "Realtek" : vid == 0x0e8dU ? "MediaTek" :
           vid == 0x148fU ? "Ralink/MediaTek" : vid == 0x0cf3U ? "Qualcomm Atheros" :
           vid == 0x1d6bU ? "Linux (hub)" : "inconnu";
}

/* Entree la plus recente en premier. Affiche aussi les mots bruts de
 * usb_attach_arg: ils aident a diagnostiquer les champs sentinelles (FFFFFFFF).
 * Ce rapport ne lit PAS les descripteurs USB ni les endpoints. */
static inline unsigned spy_describe(const SpyResident *r, uint32_t ivars_off_ok,
                                    char *out, size_t cap)
{
    uint32_t count = r->head[0];
    size_t n = 0;
    unsigned shown = 0;
    if (!ivars_off_ok) {
        snprintf(out, cap, "Pilote espion non installe.");
        return 0;
    }
    if (count == 0) {
        snprintf(out, cap,
                 "Pilote espion installe, mais le systeme n'a rien sonde.\n"
                 "Debranche l'antenne du cable OTG, rebranche-la, attends 3 s, puis \"Relire\".");
        return 0;
    }
    n += (size_t)snprintf(out + n, cap - n, "Sondages recus: %lu\n", (unsigned long)count);
    for (uint32_t k = 0; k < SPY_RING && k < count && n < cap; ++k) {
        const uint32_t *e = r->ring[(count - 1U - k) % SPY_RING];
        uint32_t st = e[E_STAGE];
        n += (size_t)snprintf(out + n, cap - n, "#%lu: ", (unsigned long)(count - k));
        if (st < 4U) {
            n += (size_t)snprintf(out + n, cap - n,
                "lecture arretee (etape %lu/4) dev=%08lX arg=%08lX\n",
                (unsigned long)st, (unsigned long)e[E_DEV], (unsigned long)e[E_ARG]);
            continue;
        }
        unsigned vid = e[E_VID] & 0xffffU, pid = e[E_PID] & 0xffffU;
        unsigned w = e[E_DEVWORD];
        n += (size_t)snprintf(out + n, cap - n,
            "VID:PID %04X:%04X (%s) if=%lu cfg=%lu adr=%u vit=%s\n"
            "brut: port=%08lX rev=%08lX dev=%08lX iface*=%08lX%s\n",
            vid, pid, spy_vendor_name(vid),
            (unsigned long)e[E_IFACE], (unsigned long)e[E_CONF],
            w & 0xffU, spy_speed_name((w >> 24) & 0xffU),
            (unsigned long)e[E_PORT], (unsigned long)e[E_REL],
            (unsigned long)e[E_DEVICE], (unsigned long)e[E_IFH],
            (e[E_IFACE] == 0xffffffffU || e[E_CONF] == 0xffffffffU)
                ? " [champ invalide?]" : "");
        ++shown;
    }
    return shown;
}
#endif
