/* CX Internet - Probe 9: inspection bornee de usb_attach_arg.
 * Aucun endpoint, aucune commande USB, aucun pointeur imbrique dereference.
 */
#include <os.h>
#include <libndls.h>
#include <usbdi.h>
#include <syscall-decls.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "usbprobe_decode.h"
#include "usb_spy8.h"

extern const uint8_t cxi_spy9_start[], cxi_spy9_end[], cxi_spy9_context[];
extern const uint8_t cxi_spy9_match[], cxi_spy9_nop[];
#define TITLE "CX Internet - USB Probe 9"
#define REG(a) (*(volatile uint32_t *)(uintptr_t)(a))

static unsigned irq_off(void) {
    unsigned old, now;
    __asm__ volatile("mrs %0,cpsr\n\torr %1,%0,#0xc0\n\tmsr cpsr_c,%1"
        : "=r"(old), "=r"(now) :: "memory", "cc");
    return old;
}
static void irq_restore(unsigned old) {
    unsigned now;
    __asm__ volatile("mrs %0,cpsr\n\tbic %0,%0,#0xc0\n\tand %1,%1,#0xc0\n\torr %0,%0,%1\n\tmsr cpsr_c,%0"
        : "=&r"(now), "+r"(old) :: "memory", "cc");
}
#define EXPORT(name, number) \
static uint32_t export_##name(void) { \
    unsigned mask=irq_off(); register uint32_t v __asm__("r0"); \
    __asm__ volatile("swi %[n]" : "=r"(v) : [n] "i"(__SYSCALLS_ISVAR | (number)) \
        : "r1","r2","r3","r4","r12","lr","memory","cc"); \
    irq_restore(mask); return v; \
}
EXPORT(ivars, e_device_get_ivars)
EXPORT(alloc, e_malloc)

static void fail(const char *s, uint32_t a, uint32_t b) {
    char t[256];
    snprintf(t,sizeof t,"%s\n\nAucune commande USB envoyee.\n%08lX %08lX",
        s,(unsigned long)a,(unsigned long)b);
    show_msgbox(TITLE,t);
}
static SpyResident *install(unsigned *offset) {
    if (!nl_hassyscall(usb_register_driver) || !nl_hassyscall(device_get_ivars)) {
        fail("Fonctions USB Ndless indisponibles.",0,0); return NULL;
    }
    uint32_t getter=export_ivars(), allocator=export_alloc();
    if ((getter&3U)||getter<0x10000000U||getter>=0x14000000U||
        (allocator&3U)||allocator<0x10000000U||allocator>=0x14000000U) {
        fail("Adresses systeme inattendues.",getter,allocator); return NULL;
    }
    if (!spy_decode_getter(REG(getter),REG(getter+4U),offset)) {
        fail("Getter device_get_ivars non reconnu.",REG(getter),REG(getter+4U));
        return NULL;
    }
    size_t bytes=(size_t)(cxi_spy9_end-cxi_spy9_start);
    size_t codeoff=(sizeof(SpyResident)+31U)&~(size_t)31U;
    uint8_t *block=((void *(*)(size_t))(uintptr_t)allocator)(codeoff+bytes);
    if (!block || ((uintptr_t)block&3U)) {
        fail("Allocation du contexte impossible.",(uint32_t)(uintptr_t)block,0);
        return NULL;
    }
    memset(block,0,codeoff+bytes);
    SpyResident *resident=(SpyResident *)block;
    resident->head[1]=*offset;
    uint8_t *code=block+codeoff;
    memcpy(code,cxi_spy9_start,bytes);
    *(uint32_t *)(code+(cxi_spy9_context-cxi_spy9_start))=(uint32_t)(uintptr_t)block;
    void *nop=code+(cxi_spy9_nop-cxi_spy9_start);
    resident->methods[0]=code+(cxi_spy9_match-cxi_spy9_start);
    resident->methods[1]=nop; resident->methods[2]=nop; resident->methods[6]=nop;
    strcpy(resident->name,"cxi-spy9");
    clear_cache();
    if (usb_register_driver(2,(int (**)(device_t))resident->methods,resident->name,0,64)) {
        fail("Enregistrement du moniteur refuse.",0,0); return NULL;
    }
    return resident;
}
static void report(const SpyResident *r,char *out,size_t cap) {
    uint32_t count=r->head[0]; size_t n=0; unsigned found=0;
    n+=(size_t)snprintf(out+n,cap-n,"Probe 9 - snapshot usb_attach_arg\nSondages: %lu\n",
        (unsigned long)count);
    for (unsigned k=0;k<SPY_RING && k<count && n<cap-180;k++) {
        const uint32_t *e=r->ring[(count-1U-k)%SPY_RING];
        if (e[13]!=4U) continue;
        ++found;
        n+=(size_t)snprintf(out+n,cap-n,
            "Capture #%lu dev=%08lX arg=%08lX\n"
            "port=%ld cfg=%ld if=%ld\nVID:PID=%04lX:%04lX rev=%04lX\n"
            "device=%08lX iface=%08lX\nusegeneric=%ld ifaces=%08lX nifaces=%ld\n",
            (unsigned long)(count-k),(unsigned long)e[0],(unsigned long)e[1],
            (long)e[2],(long)e[3],(long)e[4],
            (unsigned long)(e[5]&0xffffU),(unsigned long)(e[6]&0xffffU),
            (unsigned long)(e[7]&0xffffU),(unsigned long)e[8],
            (unsigned long)e[9],(long)e[10],(unsigned long)e[11],(long)e[12]);
    }
    if (!found) snprintf(out+n,cap-n,"Aucune capture complete. Redemarre puis reessaie.");
    else snprintf(out+n,cap-n,
        "\nSnapshot limite a 44 octets. Aucun pointeur suivi.\nAucune requete USB ni lecture endpoint.");
}
int main(void) {
    char out[640]; unsigned offset=0;
    show_msgbox(TITLE,
        "Probe 9: diagnostic de la structure USB.\n\n"
        "Branche l'OTG puis valide pour installer le moniteur.\n"
        "Rebranche l'adaptateur et attends 5 secondes.\n"
        "Le programme copie uniquement 44 octets de usb_attach_arg.\n"
        "Aucun pointeur interne n'est suivi; aucune requete USB n'est envoyee.\n"
        "Redemarre la calculatrice avant de lancer cette sonde.");
    SpyResident *r=install(&offset);
    if (!r) return 0;
    show_msgbox(TITLE,"Moniteur installe. Rebranche l'adaptateur, attends 5 secondes, puis valide.");
    SpyResident snapshot; unsigned mask=irq_off();
    memcpy(snapshot.head,r->head,sizeof snapshot.head);
    memcpy(snapshot.ring,r->ring,sizeof snapshot.ring);
    irq_restore(mask);
    report(&snapshot,out,sizeof out);
    show_msgbox(TITLE,out);
    return 0;
}
