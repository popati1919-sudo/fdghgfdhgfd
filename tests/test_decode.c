#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/usbprobe_decode.h"
int main(void)
{
    ProbeMap cx, cx2; char t[512];
    assert(probe_map_for_asic(0x00000101U, &cx) && cx.classic);
    assert(probe_map_for_asic(0x10000101U, &cx) && cx.classic);
    assert(probe_map_for_asic(0x202U, &cx2) && !cx2.classic);
    assert(!probe_map_for_asic(0x1234U, &cx));
    assert(cx.capability == 0xb0000100U && cx.port == 0xb0000184U && cx.otg == 0xb00001a4U);

    /* CX d'origine: cap valide, bit ID (1<<8) leve = mode peripherique. */
    ProbeSample s = {0x01000040U, 0, 0, 0, 1U << 8};
    assert(probe_verdict(&cx, &s) == PROBE_NO_ADAPTER);
    /* Adaptateur: bit ID retombe, port sans connexion. */
    s.otg = 0; s.port = 0x1000U;
    assert(probe_verdict(&cx, &s) == PROBE_ADAPTER_NO_DEVICE);
    /* Appareil connecte + active + alimente. */
    s.port = 0x1005U;
    assert(probe_verdict(&cx, &s) == PROBE_DEVICE_PRESENT);
    /* Controleur endormi: tout a 0 ou a 1 ne prouve rien. */
    ProbeSample z = {0, 0, 0, 0, 0};
    assert(probe_verdict(&cx, &z) == PROBE_UNREADABLE);
    ProbeSample f = {UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX};
    assert(probe_verdict(&cx, &f) == PROBE_UNREADABLE);
    /* CX II: cap 0 et otg non nul est lisible; bit 21 = peripherique. */
    ProbeSample c2 = {0, 0, 0, 0, 1U << 21};
    assert(probe_verdict(&cx2, &c2) == PROBE_NO_ADAPTER);
    c2.otg = 0x10; 
    assert(probe_verdict(&cx2, &c2) == PROBE_ADAPTER_NO_DEVICE);
    /* Le texte tient dans le tampon et reste ASCII. */
    probe_describe(0x101U, &cx, &s, t, sizeof t);
    assert(strlen(t) < sizeof t - 1);
    for (const unsigned char *p = (const unsigned char *)t; *p; ++p) assert(*p < 128);
    puts(t);
    puts("OK: tous les tests passent");
    return 0;
}
