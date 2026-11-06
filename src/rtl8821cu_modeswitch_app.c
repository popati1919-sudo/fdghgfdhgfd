/* RTL8821CU mode switch 0.3 - TI-Nspire CX / Ndless.
 * Enumerates every Realtek interface observed after driver registration.
 * A SCSI eject is attempted only after a real 0BDA:1A2B interface descriptor
 * proves Mass Storage / SCSI / BOT and both bulk endpoints are present.
 *
 * IMPORTANT: Ndless driver callbacks are event-driven. If the adapter was
 * already plugged in before registration, unplug/replug it to trigger match.
 * This is a mode-switch helper, not an RTL8821CU radio driver.
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

#include "scsi_eject.h"

#define RTL_VID                 0x0bda
#define RTL_PID_CDROM           0x1a2b
#define RTL_PID_WIFI            0xc820
#define USB_CLASS_MASS_STORAGE  0x08
#define USB_SUBCLASS_SCSI       0x06
#define USB_PROTOCOL_BOT        0x50
#define USB_CLASS_VENDOR        0xff
#define BOT_CSW_SIZE            13
#define BOT_TIMEOUT_MS          2500
#define BOT_TAG                 0x524c5443U
#define NOT_REPORTED            (-999)

typedef struct {
    int attached;
    int mode;
    int last_status;
    uint8_t bulk_in;
    uint8_t bulk_out;
} rtl_mode_sc_t;

static volatile int g_match_calls;
static volatile int g_attach_calls;
static volatile int g_cd_match;
static volatile int g_eject_attempted;
static volatile int g_eject_result = NOT_REPORTED;
static volatile int g_wifi_match;
static volatile int g_wifi_attached;
static volatile int g_last_pid = -1;
static volatile int g_last_if = -1;
static volatile int g_last_class = -1;
static volatile int g_last_subclass = -1;
static volatile int g_last_proto = -1;
static volatile int g_endpoint_count = -1;
static volatile int g_endpoint_status = -1;
static volatile uint8_t g_bulk_in;
static volatile uint8_t g_bulk_out;
static volatile int g_realtek_interfaces;

static int rtl_mode_match(device_t self)
{
    struct usb_attach_arg *uaa =
        (struct usb_attach_arg *)device_get_ivars(self);
    usb_interface_descriptor_t *id;

    if (uaa == NULL)
        return UMATCH_NONE;

    if (uaa->vendor != RTL_VID)
        return UMATCH_NONE;

    /* Do not assume the CD interface is IF0 or the WLAN interface is IF2.
     * Record all Realtek interfaces and let attach() validate their class. */
    ++g_match_calls;
    g_last_pid = uaa->product;
    g_last_if = uaa->ifaceno;

    /* A whole-device match has ifaceno == -1 and iface == NULL.
     * Returning UMATCH_VENDOR_PRODUCT (13) here binds the driver to the
     * device itself, which is exactly why attach() saw no interface.
     * Require a real interface and return the interface-specific match
     * code (8), so the USB framework invokes us per interface. */
    if (uaa->iface == NULL || uaa->ifaceno < 0)
        return UMATCH_NONE;

    id = usbd_get_interface_descriptor(uaa->iface);
    if (id == NULL)
        return UMATCH_NONE;

    ++g_realtek_interfaces;
    g_last_class = id->bInterfaceClass;
    g_last_subclass = id->bInterfaceSubClass;
    g_last_proto = id->bInterfaceProtocol;

    if (uaa->product == RTL_PID_CDROM)
        g_cd_match = 1;
    if (uaa->product == RTL_PID_WIFI)
        g_wifi_match = 1;

    return UMATCH_VENDOR_PRODUCT_CONF_IFACE;
}

static usbd_status rtl_bulk_once(usbd_device_handle dev,
                                 usbd_pipe_handle pipe,
                                 void *buffer, uint32_t length,
                                 uint32_t *actual)
{
    usbd_xfer_handle xfer;
    usbd_status status;

    if (actual != NULL)
        *actual = 0;
    xfer = usbd_alloc_xfer(dev);
    if (xfer == NULL)
        return USBD_NOMEM;
    usbd_setup_xfer(xfer, pipe, NULL, buffer, length, 0,
                    BOT_TIMEOUT_MS, NULL);
    status = usbd_sync_transfer(xfer);
    usbd_get_xfer_status(xfer, NULL, NULL, actual, NULL);
    usbd_free_xfer(xfer);
    return status;
}

static int rtl_send_scsi_eject(struct usb_attach_arg *uaa,
                               rtl_mode_sc_t *sc)
{
    usbd_status status;
    usbd_pipe_handle pipe_in = NULL, pipe_out = NULL;
    uint8_t endpoint_count = 0, ep_in = 0, ep_out = 0, i;
    uint8_t cbw[CXI_CBW_SIZE], csw[BOT_CSW_SIZE];
    uint32_t actual = 0;
    int result = -1;

    if (uaa == NULL || uaa->iface == NULL || uaa->device == NULL)
        return -10;

    status = usbd_endpoint_count(uaa->iface, &endpoint_count);
    g_endpoint_status = (int)status;
    g_endpoint_count = (int)endpoint_count;
    if (status != USBD_NORMAL_COMPLETION || endpoint_count < 2)
        return -11;

    for (i = 0; i < endpoint_count; ++i) {
        usb_endpoint_descriptor_t *ed =
            usbd_get_endpoint_descriptor(uaa->iface, i);
        if (ed == NULL || UE_GET_XFERTYPE(ed->bmAttributes) != UE_BULK)
            continue;
        if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN)
            ep_in = ed->bEndpointAddress;
        else
            ep_out = ed->bEndpointAddress;
    }
    if (ep_in == 0 || ep_out == 0)
        return -12;

    sc->bulk_in = ep_in;
    sc->bulk_out = ep_out;
    g_bulk_in = ep_in;
    g_bulk_out = ep_out;

    status = usbd_open_pipe(uaa->iface, ep_out, 0, &pipe_out);
    if (status != USBD_NORMAL_COMPLETION)
        return -20 - (int)status;
    status = usbd_open_pipe(uaa->iface, ep_in, 0, &pipe_in);
    if (status != USBD_NORMAL_COMPLETION) {
        usbd_close_pipe(pipe_out);
        return -40 - (int)status;
    }

    cxi_make_eject_cbw(cbw, BOT_TAG);
    if (!cxi_validate_eject_cbw(cbw, sizeof(cbw))) {
        result = -60;
        goto cleanup;
    }

    status = rtl_bulk_once(uaa->device, pipe_out, cbw,
                           (uint32_t)sizeof(cbw), &actual);
    if (status != USBD_NORMAL_COMPLETION || actual != sizeof(cbw)) {
        result = -70 - (int)status;
        goto cleanup;
    }

    memset(csw, 0, sizeof(csw));
    status = rtl_bulk_once(uaa->device, pipe_in, csw,
                           (uint32_t)sizeof(csw), &actual);
    if (status != USBD_NORMAL_COMPLETION || actual != sizeof(csw)) {
        result = -90 - (int)status;
        goto cleanup;
    }

    if (csw[0] != 'U' || csw[1] != 'S' ||
        csw[2] != 'B' || csw[3] != 'S' ||
        csw[4] != (uint8_t)BOT_TAG ||
        csw[5] != (uint8_t)(BOT_TAG >> 8) ||
        csw[6] != (uint8_t)(BOT_TAG >> 16) ||
        csw[7] != (uint8_t)(BOT_TAG >> 24)) {
        result = -100;
        goto cleanup;
    }
    if (csw[8] != 0 || csw[9] != 0 ||
        csw[10] != 0 || csw[11] != 0) {
        result = -103;
        goto cleanup;
    }
    result = csw[12] == 0 ? 1 : (csw[12] == 1 ? -101 : -102);

cleanup:
    if (pipe_in != NULL) usbd_close_pipe(pipe_in);
    if (pipe_out != NULL) usbd_close_pipe(pipe_out);
    return result;
}

static int rtl_mode_attach(device_t self)
{
    struct usb_attach_arg *uaa =
        (struct usb_attach_arg *)device_get_ivars(self);
    rtl_mode_sc_t *sc =
        (rtl_mode_sc_t *)device_get_softc(self);
    usb_interface_descriptor_t *id;

    ++g_attach_calls;
    if (uaa == NULL || sc == NULL || uaa->iface == NULL)
        return -1;
    id = usbd_get_interface_descriptor(uaa->iface);
    if (id == NULL)
        return -1;

    memset(sc, 0, sizeof(*sc));
    sc->attached = 1;
    g_last_pid = uaa->product;
    g_last_if = uaa->ifaceno;
    g_last_class = id->bInterfaceClass;
    g_last_subclass = id->bInterfaceSubClass;
    g_last_proto = id->bInterfaceProtocol;

    if (uaa->product == RTL_PID_CDROM) {
        g_cd_match = 1;
        if (id->bInterfaceClass != USB_CLASS_MASS_STORAGE ||
            id->bInterfaceSubClass != USB_SUBCLASS_SCSI ||
            id->bInterfaceProtocol != USB_PROTOCOL_BOT) {
            g_eject_attempted = 1;
            g_eject_result = -15;
            sc->last_status = -15;
            return 0;
        }
        /* One attempt only, and only after descriptors prove SCSI/BOT. */
        if (!g_eject_attempted) {
            g_eject_attempted = 1;
            g_eject_result = rtl_send_scsi_eject(uaa, sc);
            sc->last_status = g_eject_result;
        }
        return 0;
    }

    if (uaa->product == RTL_PID_WIFI) {
        g_wifi_match = 1;
        if (id->bInterfaceClass == USB_CLASS_VENDOR)
            g_wifi_attached = 1;
        return 0;
    }
    return 0;
}

static int rtl_mode_detach(device_t self)
{
    rtl_mode_sc_t *sc =
        (rtl_mode_sc_t *)device_get_softc(self);
    if (sc != NULL) sc->attached = 0;
    return 0;
}

static int (*rtl_mode_methods[])(device_t) = {
    rtl_mode_match, rtl_mode_attach, rtl_mode_detach, NULL
};

static const char *eject_text(int result)
{
    if (result == NOT_REPORTED) return "NON TENTEE";
    if (result == 1) return "CSW valide : commande acceptee";
    if (result == -15) return "interface detectee, mais pas SCSI/BOT";
    if (result == -11) return "pas assez d'endpoints";
    if (result == -12) return "endpoints Bulk IN/OUT absents";
    if (result <= -90 && result >= -99)
        return "pas de CSW (deconnexion ou erreur de transfert)";
    if (result == -101) return "commande SCSI refusee";
    if (result == -102) return "erreur de phase BOT";
    if (result < 0) return "echec (code numerique affiche)";
    return "etat inconnu";
}

static void show_report(int registration)
{
    char report[1100];
    snprintf(report, sizeof(report),
        "RTL8821CU - MODE SWITCH 0.4\n"
        "------------------------------\n"
        "Enregistrement Ndless : %s (%d)\n"
        "Callbacks match / attach : %d / %d\n"
        "Interfaces Realtek lues : %d\n\n"
        "Dernier PID / interface : %04X / %d\n"
        "Classe/Subclasse/Proto : %02X/%02X/%02X\n"
        "Endpoints annonces : %d (statut %d)\n"
        "Bulk IN/OUT : 0x%02X / 0x%02X\n\n"
        "Mode CD 0BDA:1A2B : %s\n"
        "Ejection SCSI : %s (code %d)\n"
        "Mode Wi-Fi 0BDA:C820 : %s\n\n"
        "IMPORTANT : si l'antenne etait deja branchee au lancement,\n"
        "debranche-la, attends 3 secondes, puis rebranche-la.\n"
        "Le pilote Ndless recoit les evenements USB a chaud.\n"
        "Aucun scan Wi-Fi ni chargement firmware dans cette version.",
        registration == 0 ? "OK" : "ECHEC", registration,
        (int)g_match_calls, (int)g_attach_calls,
        (int)g_realtek_interfaces,
        (unsigned)(g_last_pid < 0 ? 0 : g_last_pid),
        (int)g_last_if,
        (unsigned)(g_last_class < 0 ? 0 : g_last_class),
        (unsigned)(g_last_subclass < 0 ? 0 : g_last_subclass),
        (unsigned)(g_last_proto < 0 ? 0 : g_last_proto),
        (int)g_endpoint_count, (int)g_endpoint_status,
        (unsigned)g_bulk_in, (unsigned)g_bulk_out,
        g_cd_match ? "DETECTE" : "pas detecte",
        eject_text(g_eject_result), (int)g_eject_result,
        g_wifi_attached ? "INTERFACE ATTACHEE" :
        (g_wifi_match ? "PID detecte" : "pas detecte"));
    show_msgbox("RTL8821CU Mode Switch 0.4", report);
}

int main(void)
{
    int registration, choice;
    registration = usb_register_driver(
        2, rtl_mode_methods, "rtl8821cu-modeswitch04", 0,
        (unsigned int)sizeof(rtl_mode_sc_t));
    nl_set_resident();

    for (;;) {
        show_report(registration);
        choice = show_msgbox_2b("RTL8821CU - MODE SWITCH 0.3",
            "Actualiser apres avoir branche/debranche l'antenne ?",
            "Actualiser", "Quitter");
        if (choice != 1) _exit(0);
    }
    _exit(0);
}
