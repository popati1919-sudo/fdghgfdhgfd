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
    char text[640], details[512];
    const char *previous = NULL;

    if (!probe_map_for_asic(asic, &map)) {
        snprintf(text, sizeof text,
                 "Calculatrice non reconnue (ASIC %08lX).\nCe test gere seulement CX et CX II.",
                 (unsigned long)asic);
        show_msgbox("CX Internet - Sonde USB", text);
        return 0;
    }

    show_msgbox("Sonde USB - comparaison",
                "Premiere lecture : laisse le port dans son etat actuel.\n"
                "Relire compare avec la lecture precedente. Quitter ferme la sonde.");

    for (;;) {
        ProbeSample s;
        s.cap = rd(map.capability);
        s.command = rd(map.command);
        s.status = rd(map.command + 4U);
        s.port = rd(map.port);
        s.otg = rd(map.otg);
        ProbeVerdict verdict = probe_describe(asic, &map, &s, details, sizeof details);
        const char *current;
        switch (verdict) {
        case PROBE_DEVICE_PRESENT: current = "Appareil present"; break;
        case PROBE_ADAPTER_NO_DEVICE: current = "OTG seul"; break;
        case PROBE_NO_ADAPTER: current = "Sans OTG"; break;
        default: current = "Lecture incertaine"; break;
        }
        snprintf(text, sizeof text, "Avant : %s\nMaintenant : %s\n\n%s",
                 previous ? previous : "Premiere lecture", current, details);
        previous = current;
        if (show_msgbox_2b("Sonde USB", text, "Relire", "Quitter") != 1)
            break;
    }
    return 0;
}
