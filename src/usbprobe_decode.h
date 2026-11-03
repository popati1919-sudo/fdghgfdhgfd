/* CX Internet - sonde USB (etape 1, lecture seule).
 *
 * Les adresses de registres et les masques viennent de
 * Nspire-MediaPlayer-Ndless (GigaZelensky, GPLv3), src/storage/usb/usb_registers.h
 * et usb_autoconnect.h, deja valides sur CX et CX II. Ce fichier est donc aussi
 * sous GPLv3. Il ne depend pas du SDK Ndless: on peut le tester sur PC. */
#ifndef CXI_USBPROBE_DECODE_H
#define CXI_USBPROBE_DECODE_H
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

typedef struct {
    uint32_t capability, command, port, otg, role_mask;
    int classic;                 /* 1 = CX d'origine, 0 = CX II */
} ProbeMap;

typedef struct {
    uint32_t cap, command, status, port, otg;
} ProbeSample;

typedef enum {
    PROBE_UNREADABLE = 0,        /* controleur endormi: ne prouve rien */
    PROBE_NO_ADAPTER,            /* mode peripherique: pas de cable OTG vu */
    PROBE_ADAPTER_NO_DEVICE,     /* cable OTG vu, rien de branche detecte */
    PROBE_DEVICE_PRESENT         /* cable OTG + appareil detecte */
} ProbeVerdict;

static inline int probe_is_cx(uint32_t id) { return id == 0x00000101U || id == 0x10000101U; }

static inline int probe_map_for_asic(uint32_t asic, ProbeMap *m)
{
    if (probe_is_cx(asic)) {
        *m = (ProbeMap){0xb0000100U, 0xb0000140U, 0xb0000184U, 0xb00001a4U, 1U << 8, 1};
        return 1;
    }
    if (asic == 0x202U) {
        *m = (ProbeMap){0xb0000000U, 0xb0000010U, 0xb0000030U, 0xb0000080U, 1U << 21, 0};
        return 1;
    }
    return 0;
}

static inline int probe_cap_valid(const ProbeMap *m, uint32_t cap)
{
    return (cap & 0xffff00ffU) == (m->classic ? 0x01000040U : 0x01000010U);
}

/* Meme logique que usb_registers_id_readable() du projet de reference. */
static inline int probe_id_readable(const ProbeMap *m, const ProbeSample *s)
{
    if (s->otg == UINT32_MAX) return 0;
    if (probe_cap_valid(m, s->cap)) return 1;
    if (m->classic) return (s->cap & 0xffffU) == 0xfa05U;
    return s->cap == 0 && s->otg != 0;
}

static inline ProbeVerdict probe_verdict(const ProbeMap *m, const ProbeSample *s)
{
    if (!probe_id_readable(m, s)) return PROBE_UNREADABLE;
    if (s->otg & m->role_mask) return PROBE_NO_ADAPTER;
    if (probe_cap_valid(m, s->cap) && s->port != UINT32_MAX && (s->port & 1U))
        return PROBE_DEVICE_PRESENT;
    return PROBE_ADAPTER_NO_DEVICE;
}

/* Texte ASCII sans accents (la police de la boite de dialogue ne les garantit pas). */
static inline ProbeVerdict probe_describe(uint32_t asic, const ProbeMap *m,
                                          const ProbeSample *s, char *out, size_t cap)
{
    ProbeVerdict v = probe_verdict(m, s);
    const char *line1, *line2;
    switch (v) {
    case PROBE_UNREADABLE:
        line1 = "Controleur USB endormi ou illisible.";
        line2 = "Resultat non concluant: relis, ou branche l'adaptateur et recommence.";
        break;
    case PROBE_NO_ADAPTER:
        line1 = "Cable OTG NON detecte (mode peripherique).";
        line2 = "Verifie que le cable est un vrai OTG mini-USB vers USB-A.";
        break;
    case PROBE_ADAPTER_NO_DEVICE:
        line1 = "Cable OTG detecte, mais aucun appareil vu sur le port.";
        line2 = "Probable: l'antenne n'est pas alimentee (courant insuffisant) ou mal branchee.";
        break;
    default:
        line1 = "Cable OTG detecte ET un appareil est branche.";
        line2 = "Bon signe: on peut passer a l'etape 1b (lire VID:PID).";
        break;
    }
    snprintf(out, cap,
             "%s\n%s\n\n"
             "Modele: %s (ASIC %08lX)\n"
             "cap=%08lX  cmd=%08lX\n"
             "sts=%08lX  port=%08lX\n"
             "otg=%08lX\n"
             "Port: connecte=%d active=%d alimente=%d",
             line1, line2, m->classic ? "CX" : "CX II", (unsigned long)asic,
             (unsigned long)s->cap, (unsigned long)s->command,
             (unsigned long)s->status, (unsigned long)s->port, (unsigned long)s->otg,
             (int)(s->port != UINT32_MAX && (s->port & 1U)),
             (int)(s->port != UINT32_MAX && ((s->port >> 2) & 1U)),
             (int)(s->port != UINT32_MAX && ((s->port >> 12) & 1U)));
    return v;
}
#endif
