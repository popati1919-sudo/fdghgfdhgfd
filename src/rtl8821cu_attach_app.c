/* RTL8821CU USB attach probe 0.1 - TI-Nspire CX / Ndless.
 * Registers an interface-level USB driver using the same callback shape as
 * the historical Ndless HIDn drivers. It performs descriptor reads only:
 * no endpoint pipes, control requests, bulk transfers, or firmware loading.
 */
#include <os.h>
#include <libndls.h>
#include <nucleus.h>
#include <usbdi.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define RTL_VID 0x0bda
#define RTL_PID 0xc820
#define RTL_WIFI_IFACE 2
#define RTL_IFACE_CLASS 0xff

typedef struct {
    int attached;
    int vendor;
    int product;
    int interface_number;
    int interface_class;
    int interface_subclass;
    int interface_protocol;
} rtl8821cu_softc_t;

static volatile int g_attached;
static volatile int g_last_match;
static volatile int g_last_attach;

static int rtl8821cu_match(device_t self)
{
    struct usb_attach_arg *uaa =
        (struct usb_attach_arg *)device_get_ivars(self);
    usb_interface_descriptor_t *id;

    if (uaa == NULL || uaa->iface == NULL)
        return UMATCH_NONE;

    if (uaa->vendor != RTL_VID || uaa->product != RTL_PID)
        return UMATCH_NONE;

    if (uaa->ifaceno != RTL_WIFI_IFACE)
        return UMATCH_NONE;

    id = usbd_get_interface_descriptor(uaa->iface);
    if (id == NULL || id->bInterfaceClass != RTL_IFACE_CLASS)
        return UMATCH_NONE;

    g_last_match = 1;
    return UMATCH_IFACECLASS_IFACESUBCLASS;
}

static int rtl8821cu_attach(device_t self)
{
    struct usb_attach_arg *uaa =
        (struct usb_attach_arg *)device_get_ivars(self);
    rtl8821cu_softc_t *sc =
        (rtl8821cu_softc_t *)device_get_softc(self);
    usb_interface_descriptor_t *id;

    if (uaa == NULL || sc == NULL || uaa->iface == NULL)
        return -1;

    id = usbd_get_interface_descriptor(uaa->iface);
    if (id == NULL || uaa->vendor != RTL_VID ||
        uaa->product != RTL_PID || uaa->ifaceno != RTL_WIFI_IFACE ||
        id->bInterfaceClass != RTL_IFACE_CLASS)
        return -1;

    memset(sc, 0, sizeof(*sc));
    sc->vendor = uaa->vendor;
    sc->product = uaa->product;
    sc->interface_number = id->bInterfaceNumber;
    sc->interface_class = id->bInterfaceClass;
    sc->interface_subclass = id->bInterfaceSubClass;
    sc->interface_protocol = id->bInterfaceProtocol;
    sc->attached = 1;

    g_last_attach = 1;
    g_attached = 1;
    return 0;
}

static int rtl8821cu_detach(device_t self)
{
    rtl8821cu_softc_t *sc =
        (rtl8821cu_softc_t *)device_get_softc(self);
    if (sc != NULL)
        sc->attached = 0;
    g_attached = 0;
    return 0;
}

/* Ndless HIDn uses this exact callback order. The table must be relocated
 * before registration, and the application must remain resident afterward. */
static int (*rtl8821cu_methods[])(device_t) = {
    rtl8821cu_match,
    rtl8821cu_attach,
    rtl8821cu_detach,
    NULL
};

static void show_report(int registration)
{
    char report[900];
    snprintf(report, sizeof(report),
        "RTL8821CU - ATTACHEMENT USB 0.1\n"
        "------------------------------\n"
        "Enregistrement du pilote : %s (%d)\n"
        "Cible : 0BDA:C820\n"
        "Interface cible : MI_02 / classe FF\n\n"
        "Callback match : %s\n"
        "Callback attach : %s\n"
        "Etat attachement : %s\n\n"
        "Identifiant vu auparavant sur la CX : 0BDA:1A2B\n"
        "Si la cle reste en mode CD-ROM, aucun attach C820 ne sera possible.\n\n"
        "Lecture seule : aucun pipe ouvert, aucune requete de controle,\n"
        "aucun transfert bulk, aucun firmware charge.",
        registration == 0 ? "OK" : "ECHEC",
        registration,
        g_last_match ? "APPELE / CIBLE TROUVEE" : "pas encore appele",
        g_last_attach ? "APPELE" : "pas encore appele",
        g_attached ? "ATTACHE" : "NON ATTACHE");
    show_msgbox("RTL8821CU Attach Probe 0.1", report);
}

int main(void)
{
    int registration;
    int choice;

    registration = usb_register_driver(
        2, rtl8821cu_methods, "rtl8821cu", 0,
        (unsigned int)sizeof(rtl8821cu_softc_t));

    /* USB callbacks outlive main(); keep their code and state resident. */
    nl_set_resident();

    for (;;) {
        show_report(registration);
        choice = show_msgbox_2b("RTL8821CU - SUIVI",
            "Actualiser l'etat de l'attachement USB ?",
            "Actualiser", "Quitter");
        if (choice != 1)
            _exit(0);
    }
    _exit(0);
}
