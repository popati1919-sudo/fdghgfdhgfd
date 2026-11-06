/* RTL8821CU USB attachment diagnostic 0.5 - TI-Nspire CX / Ndless.
 * Uses the NetBSD-style whole-device match supported by the SDK.
 * Records usb_attach_arg interface count/pointer values without dereferencing
 * the interface array. No SCSI, BOT, endpoint or firmware transfers are sent.
 */
#include <os.h>
#include <libndls.h>
#include <nucleus.h>
#include <usbdi.h>
#include <usb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>



#define RTL_VID 0x0bda
#define RTL_PID_CDROM 0x1a2b
#define RTL_PID_WIFI 0xc820

typedef struct {
    int attached;
} rtl_diag_sc_t;

static volatile int g_match_calls;
static volatile int g_attach_calls;
static volatile int g_detach_calls;
static volatile int g_last_pid = -1;
static volatile int g_last_if = -1;
static volatile int g_last_conf = -1;
static volatile int g_last_port = -1;
static volatile int g_cd_seen;
static volatile int g_wifi_seen;
static volatile int g_null_uaa;

static void record_uaa(struct usb_attach_arg *uaa)
{
    if (uaa == NULL) {
        ++g_null_uaa;
        return;
    }
    g_last_pid = uaa->product;
    g_last_if = uaa->ifaceno;
    g_last_conf = uaa->configno;
    g_last_port = uaa->port;
    if (uaa->product == RTL_PID_CDROM) g_cd_seen = 1;
    if (uaa->product == RTL_PID_WIFI) g_wifi_seen = 1;
}

/* Device-level match: intentionally return UMATCH_VENDOR_PRODUCT (13).
 * The 0.4 interface-only match produced match callbacks but zero attach
 * callbacks on the calculator. This diagnostic tests the SDK's device
 * attachment path and captures the interface inventory metadata safely. */
static int rtl_diag_match(device_t self)
{
    struct usb_attach_arg *uaa =
        (struct usb_attach_arg *)device_get_ivars(self);
    if (uaa == NULL) {
        ++g_null_uaa;
        return UMATCH_NONE;
    }
    if (uaa->vendor != RTL_VID)
        return UMATCH_NONE;
    ++g_match_calls;
    record_uaa(uaa);
    return UMATCH_VENDOR_PRODUCT;
}

static int rtl_diag_attach(device_t self)
{
    struct usb_attach_arg *uaa =
        (struct usb_attach_arg *)device_get_ivars(self);
    rtl_diag_sc_t *sc = (rtl_diag_sc_t *)device_get_softc(self);
    ++g_attach_calls;
    if (sc != NULL) sc->attached = 1;
    record_uaa(uaa);
    return 0;
}

static int rtl_diag_detach(device_t self)
{
    rtl_diag_sc_t *sc = (rtl_diag_sc_t *)device_get_softc(self);
    ++g_detach_calls;
    if (sc != NULL) sc->attached = 0;
    return 0;
}

static int (*rtl_mode_methods[])(device_t) = {
    rtl_diag_match, rtl_diag_attach, rtl_diag_detach, NULL
};

static void show_report(int registration)
{
    char report[1000];
    snprintf(report, sizeof(report),
        "RTL8821CU - USB DIAGNOSTIC 0.6\n"
        "------------------------------\n"
        "Enregistrement Ndless : %s (%d)\n"
        "Callbacks match/attach/detach : %d/%d/%d\n"
        "usb_attach_arg NULL : %d\n\n"
        "Dernier PID : %04X\n"
        "Port / config / interface : %d / %d / %d\n\n"
        "Mode CD 0BDA:1A2B : %s\n"
        "Mode Wi-Fi 0BDA:C820 : %s\n\n"
        "Champs nifaces/ifaces masques : ABI non validee.\n"
        "Aucun endpoint, SCSI, BOT ou firmware n'est touche.\n"
        "Branche/debranche l'antenne puis actualise.",
        registration == 0 ? "OK" : "ECHEC", registration,
        (int)g_match_calls, (int)g_attach_calls, (int)g_detach_calls,
        (int)g_null_uaa, (unsigned)(g_last_pid < 0 ? 0 : g_last_pid),
        (int)g_last_port, (int)g_last_conf, (int)g_last_if,
        g_cd_seen ? "OBSERVE" : "pas observe",
        g_wifi_seen ? "OBSERVE" : "pas observe");
    show_msgbox("RTL8821CU USB Diagnostic 0.6", report);
}

int main(void)
{
    int registration, choice;
    registration = usb_register_driver(
        2, rtl_mode_methods, "rtl8821cu-usbdiag06", 0,
        (unsigned int)sizeof(rtl_diag_sc_t));
    nl_set_resident();

    for (;;) {
        show_report(registration);
        choice = show_msgbox_2b("RTL8821CU - USB DIAGNOSTIC 0.6",
            "Actualiser apres avoir branche/debranche l'antenne ?",
            "Actualiser", "Quitter");
        if (choice != 1) _exit(0);
    }
    _exit(0);
}
