/* CX Internet - sonde USB, etape 1 (LECTURE SEULE).
 * Ne modifie aucun registre: il lit seulement l'etat du controleur USB
 * et l'affiche. Licence: GPLv3 (voir usbprobe_decode.h). */
#include <os.h>
#include <libndls.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "usbprobe_decode.h"

static uint32_t rd(uint32_t address)
{
    return *(volatile uint32_t *)(uintptr_t)address;
}

int main(void)
{
    uint32_t asic = rd(0x900a0000U);
    ProbeMap map;
    char text[512];

    if (!probe_map_for_asic(asic, &map)) {
        snprintf(text, sizeof text,
                 "Calculatrice non reconnue (ASIC %08lX).\nCe test gere seulement CX et CX II.",
                 (unsigned long)asic);
        show_msgbox("CX Internet - Sonde USB", text);
        return 0;
    }

    show_msgbox("CX Internet - Sonde USB",
                "Etape 1: lecture seule, rien n'est modifie.\n\n"
                "1) Branche le cable OTG (mini-USB vers USB-A) et l'antenne.\n"
                "2) Appuie sur Entree pour lire l'etat.\n"
                "3) Note ce qui s'affiche et envoie-le moi.");

    for (;;) {
        ProbeSample s;
        s.cap = rd(map.capability);
        s.command = rd(map.command);
        s.status = rd(map.command + 4U);
        s.port = rd(map.port);
        s.otg = rd(map.otg);
        probe_describe(asic, &map, &s, text, sizeof text);
        if (show_msgbox_2b("Sonde USB", text, "Relire", "Quitter") != 1)
            break;
    }
    return 0;
}
