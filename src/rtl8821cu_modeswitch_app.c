/* RTL8821CU mode switch 0.1 - TI-Nspire CX / Ndless.
 * Handles the Realtek virtual-CD identity 0BDA:1A2B by issuing one standard
 * USB Mass Storage Bulk-Only Transport SCSI START STOP UNIT / LOEJ command.
 * After re-enumeration, it recognizes the Wi-Fi identity 0BDA:C820.
 *
 * This is deliberately NOT the RTL8821CU radio driver. It only moves the
 * adapter out of its onboard driver-CD mode so the Wi-Fi interface can bind.
 */
#include <os.h>
#include <libndls.h>
#include <usbdi.h>
#include <usb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "scsi_eject.h"

#define RTL_VID                 0x0bda
#define RTL_PID_CDROM           0x1a2b
#define RTL_PID_WIFI            0xc820
#define USB_CLASS_MASS_STORAGE  0x08
#define USB_SUBCLASS_SCSI       0x06
#define USB_PROTOCOL_BOT        0x50
#define USB_CLASS_VENDOR        0xff
#define USB_IFACE_WIFI          2
#define BOT_CSW_SIZE            13
#define BOT_TIMEOUT_MS          2500
#define BOT_TAG                 0x524c5443U

typedef struct {
    int attached;
    int mode;
    int last_status;
    uint8_t bulk_in;
    uint8_t bulk_out;
} rtl_mode_sc_t;

static volatile int g_cd_match;
static volatile int g_eject_attempted;
static volatile int g_eject_result;
static volatile int g_wifi_match;
static volatile int g_wifi_attached;
static volatile uint8_t g_bulk_in;
static volatile uint8_t g_bulk_out;

static int rtl_mode_match(device_t self)
{
    struct usb_attach_arg *uaa =
        (struct usb_attach_arg *)device_get_ivars(self);
    usb_interface_descriptor_t *id;

    if (uaa == NULL || uaa->iface == NULL || uaa->vendor != RTL_VID)
        return UMATCH_NONE;

    id = usbd_get_interface_descriptor(uaa->iface);
    if (id == NULL)
        return UMATCH_NONE;

    if (uaa->product == RTL_PID_CDROM &&
        uaa->ifaceno == 0 &&
        id->bInterfaceClass == USB_CLASS_MASS_STORAGE &&
        id->bInterfaceSubClass == USB_SUBCLASS_SCSI &&
        id->bInterfaceProtocol == USB_PROTOCOL_BOT) {
        g_cd_match = 1;
        return UMATCH_IFACECLASS_IFACESUBCLASS_IFACEPROTO;
    }

    if (uaa->product == RTL_PID_WIFI &&
        uaa->ifaceno == USB_IFACE_WIFI &&
        id->bInterfaceClass == USB_CLASS_VENDOR) {
        g_wifi_match = 1;
        return UMATCH_IFACECLASS;
    }

    return UMATCH_NONE;
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
    usbd_pipe_handle pipe_in = NULL;
    usbd_pipe_handle pipe_out = NULL;
    uint8_t endpoint_count = 0;
    uint8_t ep_in = 0;
    uint8_t ep_out = 0;
    uint8_t i;
    uint8_t cbw[CXI_CBW_SIZE];
    uint8_t csw[BOT_CSW_SIZE];
    uint32_t actual = 0;
    int result = -1;

    if (uaa == NULL || uaa->iface == NULL || uaa->device == NULL)
        return -10;

    status = usbd_endpoint_count(uaa->iface, &endpoint_count);
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

    /* Phase 1: send one 31-byte BOT command block wrapper. */
    status = rtl_bulk_once(uaa->device, pipe_out, cbw,
                           (uint32_t)sizeof(cbw), &actual);
    if (status != USBD_NORMAL_COMPLETION || actual != sizeof(cbw)) {
        result = -70 - (int)status;
        goto cleanup;
    }

    /* Phase 2: receive the 13-byte command status wrapper. The device may
     * disconnect quickly after eject; that is reported distinctly. */
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

    if (csw[8] != 0 || csw[9] != 0 || csw[10] != 0 || csw[11] != 0) {
        result = -103;
        goto cleanup;
    }

    if (csw[12] == 0)
        result = 1;       /* command accepted */
    else if (csw[12] == 1)
        result = -101;    /* SCSI command failed */
    else
        result = -102;    /* BOT phase error */

cleanup:
    if (pipe_in != NULL)
        usbd_close_pipe(pipe_in);
    if (pipe_out != NULL)
        usbd_close_pipe(pipe_out);
    return result;
}

static int rtl_mode_attach(device_t self)
{
    struct usb_attach_arg *uaa =
        (struct usb_attach_arg *)device_get_ivars(self);
    rtl_mode_sc_t *sc =
        (rtl_mode_sc_t *)device_get_softc(self);
    usb_interface_descriptor_t *id;

    if (uaa == NULL || sc == NULL || uaa->iface == NULL)
        return -1;

    id = usbd_get_interface_descriptor(uaa->iface);
    if (id == NULL)
        return -1;

    memset(sc, 0, sizeof(*sc));
    sc->attached = 1;

    if (uaa->vendor == RTL_VID && uaa->product == RTL_PID_CDROM &&
        id->bInterfaceClass == USB_CLASS_MASS_STORAGE &&
        id->bInterfaceSubClass == USB_SUBCLASS_SCSI &&
        id->bInterfaceProtocol == USB_PROTOCOL_BOT) {
        sc->mode = 1;
        if (!g_eject_attempted) {
            g_eject_attempted = 1;
            g_eject_result = rtl_send_scsi_eject(uaa, sc);
            sc->last_status = g_eject_result;
        }
        return 0;
    }

    if (uaa->vendor == RTL_VID && uaa->product == RTL_PID_WIFI &&
        uaa->ifaceno == USB_IFACE_WIFI &&
        id->bInterfaceClass == USB_CLASS_VENDOR) {
        sc->mode = 2;
        g_wifi_attached = 1;
        return 0;
    }

    return -1;
}

static int rtl_mode_detach(device_t self)
{
    rtl_mode_sc_t *sc =
        (rtl_mode_sc_t *)device_get_softc(self);
    if (sc != NULL)
        sc->attached = 0;
    return 0;
}

/* Ndless HIDn callback ABI: match, attach, detach, NULL. */
static int (*rtl_mode_methods[])(device_t) = {
    rtl_mode_match,
    rtl_mode_attach,
    rtl_mode_detach,
    NULL
};

static const char *eject_text(int result)
{
    if (result == 1) return "CSW valide : commande eject acceptee";
    if (result == 0) return "Aucune commande envoyee";
    if (result <= -90 && result >= -99)
        return "Pas de CSW : deconnexion possible pendant le changement de mode";
    if (result == -101) return "Le peripherique a refuse la commande SCSI";
    if (result == -102) return "Erreur de phase BOT";
    if (result < 0) return "Echec du transfert BOT (code numerique affiche)";
    return "Etat inconnu";
}

static void show_report(int registration)
{
    char report[900];
    snprintf(report, sizeof(report),
        "RTL8821CU - CHANGEMENT DE MODE\n"
        "------------------------------\n"
        "Enregistrement Ndless : %s (%d)\n\n"
        "Mode CD 0BDA:1A2B : %s\n"
        "Commande SCSI eject : %s\n"
        "Code eject : %d\n"
        "Endpoints Bulk IN/OUT : 0x%02X / 0x%02X\n\n"
        "Mode Wi-Fi 0BDA:C820 : %s\n\n"
        "Cette etape ne scanne pas encore les reseaux.\n"
        "Elle tente seulement le passage CD-ROM vers Wi-Fi.\n"
        "Aucun transfert firmware ni ecriture de registre radio.",
        registration == 0 ? "OK" : "ECHEC",
        registration,
        g_cd_match ? "interface stockage detectee" : "pas detecte",
        eject_text(g_eject_result),
        g_eject_result,
        (unsigned)g_bulk_in, (unsigned)g_bulk_out,
        g_wifi_attached ? "INTERFACE ATTACHEE" :
        (g_wifi_match ? "interface detectee, attach en attente" :
                        "pas encore detecte"));
    show_msgbox("RTL8821CU Mode Switch", report);
}

int main(void)
{
    int registration;
    int choice;

    nl_relocdata((unsigned *)rtl_mode_methods,
        sizeof(rtl_mode_methods) / sizeof(rtl_mode_methods[0]) - 1);

    registration = usb_register_driver(
        2, rtl_mode_methods, "rtl8821cu-modeswitch", 0,
        (unsigned int)sizeof(rtl_mode_sc_t));

    /* The USB stack keeps calling these callbacks after main returns. */
    nl_set_resident();

    for (;;) {
        show_report(registration);
        choice = show_msgbox_2b("RTL8821CU - MODE SWITCH",
            "Actualiser l'etat du changement de mode ?",
            "Actualiser", "Quitter");
        if (choice != 1)
            break;
    }
    return 0;
}
