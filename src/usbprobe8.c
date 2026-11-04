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
#include "usb_spy8.h"

extern const uint8_t cxi_spy8_start[], cxi_spy8_end[], cxi_spy8_context[];
extern const uint8_t cxi_spy8_match[], cxi_spy8_nop[];

#define TITLE "CX Internet - USB Probe 8"
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
    size_t code_bytes = (size_t)(cxi_spy8_end - cxi_spy8_start);
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
    memcpy(code, cxi_spy8_start, code_bytes);
    *(uint32_t *)(code + (cxi_spy8_context - cxi_spy8_start)) = (uint32_t)(uintptr_t)blk;
    void *nop = code + (cxi_spy8_nop - cxi_spy8_start);
    r->methods[0] = code + (cxi_spy8_match - cxi_spy8_start);
    r->methods[1] = nop;
    r->methods[2] = nop;
    r->methods[6] = nop;
    strcpy(r->name, "cxi-spy8");
    clear_cache();
    /* Meme appel que le lecteur video. Ce bloc n'est jamais libere: le systeme
     * peut rappeler le pilote apres la fin du programme. */
    if (usb_register_driver(2, (int (**)(device_t))r->methods, r->name, 0, 64)) {
        fail("Le systeme a refuse d'enregistrer le pilote.", 0, 0);
        return NULL;
    }
    return r;
}

static void report_valid(const SpyResident *r, char *out, size_t cap)
{
    uint32_t count = r->head[0];
    size_t n = 0;
    unsigned valid = 0;
    n += (size_t)snprintf(out + n, cap - n,
                          "Probe 8 - interfaces USB\\nSondages recus: %lu\\n",
                          (unsigned long)count);
    for (uint32_t k = 0; k < SPY_RING && k < count && n < cap - 120; ++k) {
        const uint32_t *e = r->ring[(count - 1U - k) % SPY_RING];
        if (e[E_STAGE] != 4U) continue;
        unsigned vid = e[E_VID] & 0xffffU;
        unsigned pid = e[E_PID] & 0xffffU;
        if (!vid || !pid || vid == 0xffffU || pid == 0xffffU) continue;
        ++valid;
        n += (size_t)snprintf(out + n, cap - n,
            "Capture #%lu: %04X:%04X (%s)\\n"
            "if=%lu cfg=%lu nifaces(brut)=%lu\\n"
            "ifaces_ptr(brut)=%08lX\\n",
            (unsigned long)(count - k), vid, pid, spy_vendor_name(vid),
            (unsigned long)e[E_IFACE], (unsigned long)e[E_CONF],
            (unsigned long)e[E_NIFACES], (unsigned long)e[E_IFACES]);
    }
    if (!valid) {
        snprintf(out + n, cap - n,
                 "\\nAucun VID:PID complet observe. Envoie cet ecran.");
        return;
    }
    snprintf(out + n, cap - n,
             "\\nPointeur ifaces affiche seulement: jamais dereference.\\n"
             "Aucune commande USB ni lecture d'endpoint.");
}

int main(void)
{
    char report[640];
    show_msgbox(TITLE,
                "Probe 8: suivi des identifiants USB.\n\n"
                "1) Branche le cable OTG.\n"
                "2) Appuie sur Entree pour installer la sonde.\n"
                "3) Rebranche l'adaptateur Wi-Fi et attends 5 secondes.\n"
                "4) Appuie sur Entree pour afficher les captures.\n\n"
                "Les sondages incomplets sont ignores.\n"
                "Aucune commande USB ni lecture d'endpoint.\n"
                "Redemarre la calculatrice avant Probe 8 si Probe 7 a ete lance.");
    unsigned off = 0;
    SpyResident *r = install(&off);
    if (!r) return 0;
    show_msgbox(TITLE,
                "Sonde installee. Rebranche maintenant l'adaptateur Wi-Fi,\n"
                "attends 5 secondes, puis appuie sur Entree.");
    SpyResident snapshot;
    unsigned mask = irq_off();
    memcpy(snapshot.head, r->head, sizeof snapshot.head);
    memcpy(snapshot.ring, r->ring, sizeof snapshot.ring);
    irq_restore(mask);
    report_valid(&snapshot, report, sizeof report);
    show_msgbox(TITLE, report);
    return 0;
}
