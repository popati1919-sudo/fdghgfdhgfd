/* CX Internet - Probe 11: audit ABI USB Ndless, sans action materielle.
 * Verifie les declarations SDK et les offsets de usb_attach_arg a la compilation.
 * Ne lit aucun device_t, ne suit aucun pointeur, n'enregistre aucun pilote,
 * ne demande aucun descripteur et n'effectue aucun transfert USB.
 */
#include <os.h>
#include <libndls.h>
#include <usbdi.h>
#include <syscall-decls.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#define TITLE "CX Internet - USB Probe 11"
static const char *yn(int v) { return v ? "DISPONIBLE" : "ABSENTE"; }

int main(void)
{
    char report[900];
    int s_register = nl_hassyscall(usb_register_driver);
    int s_ivars = nl_hassyscall(device_get_ivars);
    int s_softc = nl_hassyscall(device_get_softc);
    int s_idesc = nl_hassyscall(usbd_get_interface_descriptor);
    int s_epdesc = nl_hassyscall(usbd_interface2endpoint_descriptor);
    int s_epcount = nl_hassyscall(usbd_endpoint_count);
    int s_openpipe = nl_hassyscall(usbd_open_pipe);
    int s_openintr = nl_hassyscall(usbd_open_pipe_intr);
    int s_request = nl_hassyscall(usbd_do_request);

    /* sizeof/offsetof sont des constantes compile-time; aucun pointeur lu. */
    snprintf(report, sizeof(report),
        "PROBE 11 - AUDIT ABI USB\nSDK Ndless / compatibilite\n\n"
        "SYSCALLS DECLAREES:\n"
        "usb_register_driver: %s\ndevice_get_ivars: %s\n"
        "device_get_softc: %s\nget_interface_descriptor: %s\n"
        "interface2endpoint_descriptor: %s\nendpoint_count: %s\n"
        "open_pipe: %s\nopen_pipe_intr: %s\ndo_request: %s\n\n"
        "SIGNATURE SDK usb_register_driver:\n"
        "int (int, int(*[])(device_t), const char*, int, unsigned int)\n"
        "Methodes attendues: match / attach / detach\n\n"
        "usb_attach_arg (ABI compilee):\nsizeof=%u octets\n"
        "port=%u configno=%u ifaceno=%u\n"
        "vendor=%u product=%u release=%u\n"
        "device=%u iface=%u usegeneric=%u\n"
        "ifaces=%u nifaces=%u\n\n"
        "CIBLE: RTL8821CU 0BDA:C820, interface MI_02\n"
        "ATTENTION: reference Windows seulement.\n\n"
        "AUCUNE ACTION MATERIELLE:\n"
        "- aucun pilote enregistre\n- aucun device_t / ivars / softc lu\n"
        "- aucun pointeur suivi\n- aucun descripteur demande\n"
        "- aucun transfert / firmware / scan Wi-Fi\n\n"
        "Presence syscall != fonction testee ou sure.",
        yn(s_register), yn(s_ivars), yn(s_softc), yn(s_idesc),
        yn(s_epdesc), yn(s_epcount), yn(s_openpipe), yn(s_openintr),
        yn(s_request), (unsigned)sizeof(struct usb_attach_arg),
        (unsigned)offsetof(struct usb_attach_arg, port),
        (unsigned)offsetof(struct usb_attach_arg, configno),
        (unsigned)offsetof(struct usb_attach_arg, ifaceno),
        (unsigned)offsetof(struct usb_attach_arg, vendor),
        (unsigned)offsetof(struct usb_attach_arg, product),
        (unsigned)offsetof(struct usb_attach_arg, release),
        (unsigned)offsetof(struct usb_attach_arg, device),
        (unsigned)offsetof(struct usb_attach_arg, iface),
        (unsigned)offsetof(struct usb_attach_arg, usegeneric),
        (unsigned)offsetof(struct usb_attach_arg, ifaces),
        (unsigned)offsetof(struct usb_attach_arg, nifaces));
    show_msgbox(TITLE, report);
    return 0;
}
