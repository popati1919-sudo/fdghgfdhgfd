/* RTL8821CU USB interface diagnostic 0.7 - TI-Nspire CX / Ndless.
 * Matches only the Wi-Fi function (0BDA:C820, interface 2, class FF).
 * Reads the interface descriptor supplied by the attach callback only.
 * No endpoints, pipes, SCSI/BOT, bulk transfers or firmware operations.
 */
#include <os.h>
#include <libndls.h>
#include <nucleus.h>
#include <usbdi.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define RTL_VID 0x0bda
#define RTL_PID_WIFI 0xc820
#define RTL_WIFI_IFACE 2
#define RTL_IFACE_CLASS 0xff

typedef struct {
    int attached, vendor, product, configuration;
    int interface_number, interface_class, interface_subclass, interface_protocol;
} rtl8821cu_diag_sc_t;

static volatile int g_match, g_attach, g_detach, g_bad_args;
static volatile int g_pid=-1, g_config=-1, g_if=-1;
static volatile int g_class=-1, g_subclass=-1, g_protocol=-1;

static void record_interface(struct usb_attach_arg *uaa,
                             usb_interface_descriptor_t *id)
{
    g_pid=uaa->product; g_config=uaa->configno;
    g_if=id->bInterfaceNumber; g_class=id->bInterfaceClass;
    g_subclass=id->bInterfaceSubClass; g_protocol=id->bInterfaceProtocol;
}

static int rtl_match(device_t self)
{
    struct usb_attach_arg *uaa=(struct usb_attach_arg *)device_get_ivars(self);
    usb_interface_descriptor_t *id;
    if (!uaa || !uaa->iface) { ++g_bad_args; return UMATCH_NONE; }
    if (uaa->vendor!=RTL_VID || uaa->product!=RTL_PID_WIFI ||
        uaa->ifaceno!=RTL_WIFI_IFACE) return UMATCH_NONE;
    id=usbd_get_interface_descriptor(uaa->iface);
    if (!id) { ++g_bad_args; return UMATCH_NONE; }
    if (id->bInterfaceClass!=RTL_IFACE_CLASS) return UMATCH_NONE;
    record_interface(uaa,id); ++g_match;
    return UMATCH_IFACECLASS;
}

static int rtl_attach(device_t self)
{
    struct usb_attach_arg *uaa=(struct usb_attach_arg *)device_get_ivars(self);
    rtl8821cu_diag_sc_t *sc=(rtl8821cu_diag_sc_t *)device_get_softc(self);
    usb_interface_descriptor_t *id;
    if (!uaa || !sc || !uaa->iface) { ++g_bad_args; return -1; }
    id=usbd_get_interface_descriptor(uaa->iface);
    if (!id || uaa->vendor!=RTL_VID || uaa->product!=RTL_PID_WIFI ||
        uaa->ifaceno!=RTL_WIFI_IFACE || id->bInterfaceClass!=RTL_IFACE_CLASS) {
        ++g_bad_args; return -1;
    }
    memset(sc,0,sizeof(*sc));
    sc->attached=1; sc->vendor=uaa->vendor; sc->product=uaa->product;
    sc->configuration=uaa->configno; sc->interface_number=id->bInterfaceNumber;
    sc->interface_class=id->bInterfaceClass;
    sc->interface_subclass=id->bInterfaceSubClass;
    sc->interface_protocol=id->bInterfaceProtocol;
    record_interface(uaa,id); ++g_attach;
    return 0;
}

static int rtl_detach(device_t self)
{
    rtl8821cu_diag_sc_t *sc=(rtl8821cu_diag_sc_t *)device_get_softc(self);
    if (sc) sc->attached=0;
    ++g_detach; return 0;
}

static int (*rtl_methods[])(device_t)={rtl_match,rtl_attach,rtl_detach,NULL};

static void show_report(int registration)
{
    char report[1000];
    snprintf(report,sizeof(report),
        "RTL8821CU - INTERFACE USB 0.7\n"
        "------------------------------\n"
        "Enregistrement Ndless : %s (%d)\n"
        "Cible : 0BDA:C820 / interface 2\n"
        "Classe cible : FF\n\n"
        "Callbacks match/attach/detach : %d/%d/%d\n"
        "Arguments/descripteur invalides : %d\n\n"
        "PID / configuration / interface : %04X / %d / %d\n"
        "Classe / sous-classe / protocole : %02X / %02X / %02X\n\n"
        "Si aucun match : adaptateur probablement encore en mode CD-ROM 1A2B.\n"
        "Aucun endpoint/pipe, SCSI/BOT, transfert bulk ou firmware.",
        registration==0?"OK":"ECHEC",registration,
        (int)g_match,(int)g_attach,(int)g_detach,(int)g_bad_args,
        (unsigned)(g_pid<0?0:g_pid),(int)g_config,(int)g_if,
        (unsigned)(g_class<0?0:g_class),
        (unsigned)(g_subclass<0?0:g_subclass),
        (unsigned)(g_protocol<0?0:g_protocol));
    show_msgbox("RTL8821CU USB Diagnostic 0.7",report);
}

int main(void)
{
    int registration,choice;
    registration=usb_register_driver(2,rtl_methods,"rtl8821cu-usbdiag07",0,
                                     (unsigned int)sizeof(rtl8821cu_diag_sc_t));
    nl_set_resident();
    for (;;) {
        show_report(registration);
        choice=show_msgbox_2b("RTL8821CU - SUIVI 0.7",
            "Actualiser apres branchement de l'adaptateur ?",
            "Actualiser","Quitter");
        if (choice!=1) _exit(0);
    }
    _exit(0);
}
