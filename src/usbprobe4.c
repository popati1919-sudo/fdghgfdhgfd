/* CX Internet - etape 1b: lire le VID:PID de l'appareil USB.
 * Methode du lecteur video (GPLv3): enregistrer un pilote aupres de la pile USB
 * du systeme. Ici le pilote est un espion: il note ce que le systeme sonde et
 * repond "pas pour moi". Il ne prend en charge aucun appareil, n'ecrit dans
 * aucun registre USB et ne reconfigure rien. Licence: GPLv3.
 *
 * Verifications strictes: au moindre doute le programme s'arrete avec un message
 * et n'enregistre rien. */
#include <os.h>
#include <libndls.h>
#include <usbdi.h>
#include <syscall-decls.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "usbprobe_decode.h"
#include "usb_spy.h"

extern const uint8_t cxi_spy_start[], cxi_spy_end[], cxi_spy_context[];
extern const uint8_t cxi_spy_match[], cxi_spy_nop[];

#define TITLE "CX Internet - USB Probe 4"
#define REG(a) (*(volatile uint32_t *)(uintptr_t)(a))

static unsigned irq_off(void)
{
    unsigned old, now;
    __asm__ volatile("mrs %0,cpsr\n\torr %1,%0,#0xc0\n\tmsr cpsr_c,%1" : "=r"(old), "=r"(now) :: "memory", "cc");
    return old;
}

static void irq_restore(unsigned old)
{
    unsigned now;
    __asm__ volatile("mrs %0,cpsr\n\tbic %0,%0,#0xc0\n\tand %1,%1,#0xc0\n\torr %0,%0,%1\n\tmsr cpsr_c,%0"
                     : "=&r"(now), "+r"(old) :: "memory", "cc");
}

/* Adresse d'une fonction du systeme, comme dans le lecteur video. */
#define EXPORT(name, number)                                                           \
    static uint32_t export_##name(void)                                                \
    {                                                                                  \
        unsigned mask = irq_off();                                                     \
        register uint32_t v __asm__("r0");                                             \
        __asm__ volatile("swi %[n]" : "=r"(v) : [n] "i"(__SYSCALLS_ISVAR | (number))   \
                         : "r1", "r2", "r3", "r4", "r12", "lr", "memory", "cc");       \
        uint32_t result = v;                                                           \
        irq_restore(mask);                                                             \
        return result;                                                                 \
    }
EXPORT(ivars, e_device_get_ivars)
EXPORT(alloc, e_malloc)

static uint32_t rd_port(const ProbeMap *m) { return REG(m->port); }

static uint32_t fail(const char *what, uint32_t a, uint32_t b)
{
    char t[300];
    snprintf(t, sizeof t, "%s\n\nRien n'a ete enregistre ni modifie.\nValeurs: %08lX %08lX\n"
             "Envoie-moi cet ecran.", what, (unsigned long)a, (unsigned long)b);
    show_msgbox(TITLE, t);
    return 0;
}

static SpyResident *install(unsigned *ivars_offset)
{
    if (!nl_hassyscall(usb_register_driver) || !nl_hassyscall(device_get_ivars)) {
        fail("Ce systeme n'expose pas les fonctions USB necessaires.", 0, 0);
        return NULL;
    }
    uint32_t g = export_ivars(), m = export_alloc();
    if ((g & 3U) || g < 0x10000000U || g >= 0x14000000U || (m & 3U) || m < 0x10000000U ||
        m >= 0x14000000U) {
        fail("Adresses systeme inattendues.", g, m);
        return NULL;
    }
    uint32_t w0 = REG(g), w1 = REG(g + 4U);
    if (!spy_decode_getter(w0, w1, ivars_offset)) {
        fail("La fonction device_get_ivars de ce systeme n'a pas la forme attendue.", w0, w1);
        return NULL;
    }
    size_t code_bytes = (size_t)(cxi_spy_end - cxi_spy_start);
    size_t code_offset = (sizeof(SpyResident) + 31U) & ~(size_t)31U;
    uint8_t *blk = ((void *(*)(size_t))(uintptr_t)m)(code_offset + code_bytes);
    if (!blk || ((uintptr_t)blk & 3U)) {
        fail("Memoire insuffisante pour le pilote espion.", (uint32_t)(uintptr_t)blk, 0);
        return NULL;
    }
    memset(blk, 0, code_offset + code_bytes);
    SpyResident *r = (SpyResident *)blk;
    r->head[1] = *ivars_offset;
    uint8_t *code = blk + code_offset;
    memcpy(code, cxi_spy_start, code_bytes);
    *(uint32_t *)(code + (cxi_spy_context - cxi_spy_start)) = (uint32_t)(uintptr_t)blk;
    void *nop = code + (cxi_spy_nop - cxi_spy_start);
    r->methods[0] = code + (cxi_spy_match - cxi_spy_start);
    r->methods[1] = nop;
    r->methods[2] = nop;
    r->methods[6] = nop;
    strcpy(r->name, "cxi-spy");
    clear_cache();
    /* Meme appel que le lecteur video. Ce bloc n'est jamais libere: le systeme
     * peut rappeler le pilote apres la fin du programme. */
    if (usb_register_driver(2, (int (**)(device_t))r->methods, r->name, 0, 64)) {
        fail("Le systeme a refuse d'enregistrer le pilote.", 0, 0);
        return NULL;
    }
    return r;
}

int main(void)
{
    uint32_t asic = REG(0x900a0000U);
    ProbeMap map;
    char text[640];

    if (!probe_map_for_asic(asic, &map)) {
        fail("Calculatrice non reconnue (CX et CX II seulement).", asic, 0);
        return 0;
    }
    show_msgbox(TITLE,
                "Etape 1c: relire les informations USB en memoire.\n\n"
                "1) Branche le cable OTG mini-USB vers USB-A.\n"
                "2) Appuie sur Entree: un pilote espion est installe.\n"
                "   Il ne prend en charge aucun appareil.\n"
                "3) Debranche puis rebranche l'antenne, attends 3 s.\n"
                "4) Choisis \"Relire\" pour afficher les informations capturees.\n   Cette version ne lit pas encore les endpoints.\n\n"
                "Le pilote reste installe jusqu'au redemarrage. Ne lance pas\n"
                "ce programme deux fois sans redemarrer.");
    unsigned off = 0;
    SpyResident *r = install(&off);
    if (!r)
        return 0;
    for (;;) {
        /* Lecture prudente des seules donnees copiees par le pilote espion.
         * Ne jamais dereferencer E_IFH: ce pointeur peut etre invalide apres
         * le sondage et les getters USB peuvent figer la calculatrice. */
        /* Snapshot atomique du petit tampon partage avec le pilote.
         * Les interruptions sont masquees pendant la copie afin d'eviter
         * qu'un callback USB ne modifie l'anneau au milieu de la lecture. */
        SpyResident snapshot;
        unsigned mask = irq_off();
        memcpy(snapshot.head, r->head, sizeof snapshot.head);
        memcpy(snapshot.ring, r->ring, sizeof snapshot.ring);
        irq_restore(mask);
        spy_describe(&snapshot, 1, text, sizeof text);
        size_t n = strlen(text);
        snprintf(text + n, sizeof text - n, "\nport=%08lX otg=%08lX",
                 (unsigned long)rd_port(&map), (unsigned long)REG(map.otg));
        if (show_msgbox_2b(TITLE, text, "Relire", "Quitter") != 1)
            break;
    }
    return 0;
}
