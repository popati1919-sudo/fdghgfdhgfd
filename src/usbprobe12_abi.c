/* CX Internet - Probe 12: assertions ABI basees sur les vrais headers Ndless.
 * Verification compile-time uniquement. Aucune operation materielle USB.
 */
#include <os.h>
#include <libndls.h>
#include <usbdi.h>
#include <syscall-decls.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define TITLE "CX Internet - USB Probe 12"
#define ABI_ASSERT(name, expr) typedef char abi_assert_##name[(expr) ? 1 : -1]

/* Le SDK Ndless pour TI-Nspire est une cible ARM 32 bits. */
ABI_ASSERT(pointer_is_32_bit, sizeof(void *) == 4);
ABI_ASSERT(int_is_32_bit, sizeof(int) == 4);
ABI_ASSERT(usb_attach_arg_size, sizeof(struct usb_attach_arg) == 44);
ABI_ASSERT(offset_port, offsetof(struct usb_attach_arg, port) == 0);
ABI_ASSERT(offset_configno, offsetof(struct usb_attach_arg, configno) == 4);
ABI_ASSERT(offset_ifaceno, offsetof(struct usb_attach_arg, ifaceno) == 8);
ABI_ASSERT(offset_vendor, offsetof(struct usb_attach_arg, vendor) == 12);
ABI_ASSERT(offset_product, offsetof(struct usb_attach_arg, product) == 16);
ABI_ASSERT(offset_release, offsetof(struct usb_attach_arg, release) == 20);
ABI_ASSERT(offset_device, offsetof(struct usb_attach_arg, device) == 24);
ABI_ASSERT(offset_iface, offsetof(struct usb_attach_arg, iface) == 28);
ABI_ASSERT(offset_usegeneric, offsetof(struct usb_attach_arg, usegeneric) == 32);
ABI_ASSERT(offset_ifaces, offsetof(struct usb_attach_arg, ifaces) == 36);
ABI_ASSERT(offset_nifaces, offsetof(struct usb_attach_arg, nifaces) == 40);

/* Compare les prototypes des declarations reelles du SDK, sans les appeler. */
typedef int (*usb_register_driver_fn)(int, int (**)(device_t), const char *, int, unsigned int);
typedef void *(*device_get_ivars_fn)(device_t);
typedef void *(*device_get_softc_fn)(device_t);
typedef usb_interface_descriptor_t *(*get_interface_descriptor_fn)(usbd_interface_handle);
typedef usb_endpoint_descriptor_t *(*interface2endpoint_fn)(usbd_interface_handle, uint8_t);
typedef usbd_status (*endpoint_count_fn)(usbd_interface_handle, uint8_t *);
typedef usbd_status (*open_pipe_fn)(usbd_interface_handle, uint8_t, uint8_t, usbd_pipe_handle *);
typedef usbd_status (*do_request_fn)(usbd_device_handle, usb_device_request_t *, void *);

ABI_ASSERT(sig_usb_register_driver, __builtin_types_compatible_p(__typeof__(&usb_register_driver), usb_register_driver_fn));
ABI_ASSERT(sig_device_get_ivars, __builtin_types_compatible_p(__typeof__(&device_get_ivars), device_get_ivars_fn));
ABI_ASSERT(sig_device_get_softc, __builtin_types_compatible_p(__typeof__(&device_get_softc), device_get_softc_fn));
ABI_ASSERT(sig_get_interface_descriptor, __builtin_types_compatible_p(__typeof__(&usbd_get_interface_descriptor), get_interface_descriptor_fn));
ABI_ASSERT(sig_interface2endpoint, __builtin_types_compatible_p(__typeof__(&usbd_interface2endpoint_descriptor), interface2endpoint_fn));
ABI_ASSERT(sig_endpoint_count, __builtin_types_compatible_p(__typeof__(&usbd_endpoint_count), endpoint_count_fn));
ABI_ASSERT(sig_open_pipe, __builtin_types_compatible_p(__typeof__(&usbd_open_pipe), open_pipe_fn));
ABI_ASSERT(sig_do_request, __builtin_types_compatible_p(__typeof__(&usbd_do_request), do_request_fn));

int main(void)
{
    char report[900];
    int checks = 0;
    checks += (sizeof(void *) == 4);
    checks += (sizeof(int) == 4);
    checks += (sizeof(struct usb_attach_arg) == 44);
    checks += (offsetof(struct usb_attach_arg, vendor) == 12);
    checks += (offsetof(struct usb_attach_arg, device) == 24);
    checks += (offsetof(struct usb_attach_arg, iface) == 28);
    checks += (offsetof(struct usb_attach_arg, ifaces) == 36);
    checks += (offsetof(struct usb_attach_arg, nifaces) == 40);

    snprintf(report, sizeof(report),
        "PROBE 12 - VALIDATION ABI USB\n"
        "Headers Ndless reels / assertions compile-time\n\n"
        "RESULTAT: %d/8 controles ABI valides\n"
        "Pointeur: %u octets | int: %u octets\n"
        "usb_attach_arg: %u octets\n"
        "vendor=%u product=%u release=%u\n"
        "device=%u iface=%u usegeneric=%u\n"
        "ifaces=%u nifaces=%u\n\n"
        "Signatures verifiees a la compilation:\n"
        "- usb_register_driver\n- device_get_ivars / device_get_softc\n"
        "- usbd_get_interface_descriptor\n- usbd_interface2endpoint_descriptor\n"
        "- usbd_endpoint_count / usbd_open_pipe / usbd_do_request\n\n"
        "CIBLE: RTL8821CU 0BDA:C820, MI_02\n"
        "AUCUN appel USB, aucun pointeur lu, aucun pilote enregistre.\n"
        "ABI compilee valide != compatibilite materielle prouvee.",
        checks, (unsigned)sizeof(void *), (unsigned)sizeof(int),
        (unsigned)sizeof(struct usb_attach_arg),
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
