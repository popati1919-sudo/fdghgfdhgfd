/* Test du code resident usb_spy.S sous qemu-user ARM (pas sur la calculatrice).
 * Il recree en memoire une fausse chaine device_t -> usb_attach_arg -> device. */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "../src/usb_spy.h"

extern const uint8_t cxi_spy_start[], cxi_spy_end[], cxi_spy_context[], cxi_spy_match[], cxi_spy_nop[];
typedef int (*match_fn)(uint32_t);

#define IVOFF 0x18U
static uint8_t *ram;                 /* 0x10000000.. */
static SpyResident *res;
static match_fn match, nop;

static void put(uint32_t addr, uint32_t v) { *(uint32_t *)(ram + (addr - 0x10000000U)) = v; }

static void fake(uint32_t d, uint32_t a, uint32_t dev, uint32_t vid, uint32_t pid, uint32_t devword)
{
    put(d + IVOFF, a);
    uint32_t u[8] = {7, 1, 0, vid, pid, 0x0200, dev, 0x10009000U};
    for (int i = 0; i < 8; ++i) put(a + 4U * i, u[i]);
    if (dev >= 0x10000000U && dev < 0x100ff000U) put(dev + 8, devword);
}

int main(void)
{
    ram = mmap((void *)0x10000000U, 0x100000, PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    assert(ram == (void *)0x10000000U);
    size_t csz = (size_t)(cxi_spy_end - cxi_spy_start);
    size_t off = (sizeof(SpyResident) + 31U) & ~31U;
    uint8_t *blk = mmap(NULL, off + csz + 256, PROT_READ | PROT_WRITE | PROT_EXEC,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(blk != MAP_FAILED);
    memset(blk, 0xA5, off + csz + 256);          /* canari */
    res = (SpyResident *)blk;
    memset(res, 0, sizeof *res);
    res->head[1] = IVOFF;
    uint8_t *code = blk + off;
    memcpy(code, cxi_spy_start, csz);
    *(uint32_t *)(code + (cxi_spy_context - cxi_spy_start)) = (uint32_t)(uintptr_t)blk;
    __builtin___clear_cache((char *)code, (char *)code + csz);
    match = (match_fn)(uintptr_t)(code + (cxi_spy_match - cxi_spy_start));
    nop = (match_fn)(uintptr_t)(code + (cxi_spy_nop - cxi_spy_start));

    /* 1: chaine complete */
    fake(0x10001000U, 0x10002000U, 0x10003000U, 0x0bda, 0xc811, 0x02000A01U);
    assert(match(0x10001000U) == 0);
    assert(res->head[0] == 1);
    const uint32_t *e = res->ring[0];
    assert(e[E_STAGE] == 4 && e[E_DEV] == 0x10001000U && e[E_ARG] == 0x10002000U);
    assert(e[E_VID] == 0x0bda && e[E_PID] == 0xc811 && e[E_DEVWORD] == 0x02000A01U);
    assert(e[E_PORT] == 7 && e[E_DEVICE] == 0x10003000U);

    SpyResident snap = *res;
    /* 2: device_t mal aligne / hors RAM: aucune lecture, etape 1, rien ne plante */
    assert(match(0x10001002U) == 0 && res->ring[1][E_STAGE] == 1);
    assert(match(0x20000000U) == 0 && res->ring[2][E_STAGE] == 1);
    assert(match(0x00000000U) == 0 && res->ring[3][E_STAGE] == 1);
    /* 3: pointeur a hors plage -> etape 2 */
    put(0x10001100U + IVOFF, 0x40000000U);
    assert(match(0x10001100U) == 0 && res->ring[0][E_STAGE] == 2);   /* anneau: 5e appel */
    /* 4: device invalide -> etape 3 */
    fake(0x10001200U, 0x10002100U, 0x30000000U, 1, 2, 0);
    assert(match(0x10001200U) == 0 && res->ring[1][E_STAGE] == 3);
    assert(res->head[0] == 6);
    /* 5: l'entree ecrasee d'un tour d'anneau ne garde rien de l'ancienne */
    assert(res->ring[1][E_VID] == 1 && res->ring[1][E_DEVWORD] == 0);
    assert(nop(0x1234) == 0);

    /* canaris: rien d'ecrit hors du bloc de contexte */
    for (size_t i = sizeof(SpyResident); i < off; ++i) assert(blk[i] == 0xA5 || i >= off);
    for (size_t i = off + csz; i < off + csz + 256; ++i) assert(blk[i] == 0xA5);
    for (size_t i = 0; i < 0x100000; i += 4) {
        uint32_t v = *(uint32_t *)(ram + i);
        uint32_t a = 0x10000000U + (uint32_t)i;
        (void)v; (void)a;
    }

    char text[512];
    unsigned n = spy_describe(&snap, 1, text, sizeof text);
    assert(n == 1);
    puts(text);
    assert(strstr(text, "0BDA:C811") && strstr(text, "Realtek") && strstr(text, "vit=plein"));
    assert(spy_describe(res, 1, text, sizeof text) == 0 && strstr(text, "arretee"));
    assert(spy_describe(res, 0, text, sizeof text) == 0);
    unsigned o;
    assert(spy_decode_getter(0xe5900018U, 0xe12fff1eU, &o) && o == 0x18);
    assert(!spy_decode_getter(0xe5900100U, 0xe12fff1eU, &o));
    assert(!spy_decode_getter(0xe5900018U, 0xe1a00000U, &o));
    puts("OK: usb_spy");
    return 0;
}
