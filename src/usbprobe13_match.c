/* CX Internet - Probe 13: test logiciel de la table de correspondance USB.
 * Les structures sont locales et simulees : aucun acces au bus USB,
 * aucune lecture de device_t, aucun enregistrement de pilote.
 */
#include <os.h>
#include <libndls.h>
#include <usbdi.h>
#include <stdio.h>
#include <string.h>

#define TITLE "CX Internet - USB Probe 13"
#define RTL8821CU_VENDOR 0x0BDA
#define RTL8821CU_PRODUCT 0xC820
#define RTL8821CU_WIFI_IFACE 2

static int rtl8821cu_match(const struct usb_attach_arg *uaa)
{
    if (!uaa) return 0;
    return uaa->vendor == RTL8821CU_VENDOR &&
           uaa->product == RTL8821CU_PRODUCT &&
           uaa->ifaceno == RTL8821CU_WIFI_IFACE;
}

int main(void)
{
    struct usb_attach_arg test;
    char report[700];
    int exact, wrong_vendor, wrong_product, wrong_iface;

    memset(&test, 0, sizeof(test));
    test.vendor = RTL8821CU_VENDOR;
    test.product = RTL8821CU_PRODUCT;
    test.ifaceno = RTL8821CU_WIFI_IFACE;
    exact = rtl8821cu_match(&test);

    test.vendor = 0x1234;
    wrong_vendor = rtl8821cu_match(&test);
    test.vendor = RTL8821CU_VENDOR;
    test.product = 0x5678;
    wrong_product = rtl8821cu_match(&test);
    test.product = RTL8821CU_PRODUCT;
    test.ifaceno = 0;
    wrong_iface = rtl8821cu_match(&test);

    snprintf(report, sizeof(report),
        "PROBE 13 - TEST MATCH USB\n"
        "Table de correspondance RTL8821CU\n\n"
        "Cible attendue: 0BDA:C820\n"
        "Interface Wi-Fi: MI_02 (ifaceno=2)\n\n"
        "Test cible exacte: %s\n"
        "Mauvais fabricant refuse: %s\n"
        "Mauvais produit refuse: %s\n"
        "Mauvaise interface refuse: %s\n\n"
        "Resultat: %s\n\n"
        "ATTENTION: tests sur structure locale simulee.\n"
        "Aucun peripherique USB detecte ou contacte.\n"
        "Aucun pilote enregistre; aucun transfert; aucun scan Wi-Fi.",
        exact ? "OK" : "ECHEC",
        !wrong_vendor ? "OK" : "ECHEC",
        !wrong_product ? "OK" : "ECHEC",
        !wrong_iface ? "OK" : "ECHEC",
        (exact && !wrong_vendor && !wrong_product && !wrong_iface)
            ? "4/4 TESTS REUSSIS" : "ECHEC");
    show_msgbox(TITLE, report);
    return 0;
}
