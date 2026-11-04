/* CX Internet - analyseur de descripteurs USB standard.
 * Etape logicielle sans acces materiel: ne lance aucune transaction USB.
 * GPLv3, comme le reste du projet.
 */
#ifndef CXI_USB_DESCRIPTORS_H
#define CXI_USB_DESCRIPTORS_H
#include <stddef.h>
#include <stdint.h>
#define CXI_USB_MAX_INTERFACES 8U
#define CXI_USB_MAX_ENDPOINTS 16U
typedef struct { uint8_t number, alternate, klass, subclass, protocol, endpoint_count; } CxiUsbInterface;
typedef struct { uint8_t address, attributes, interface_number; uint16_t max_packet; uint8_t interval; } CxiUsbEndpoint;
typedef struct {
    uint16_t total_length;
    uint8_t configuration_value, interface_count, endpoint_count;
    CxiUsbInterface interfaces[CXI_USB_MAX_INTERFACES];
    CxiUsbEndpoint endpoints[CXI_USB_MAX_ENDPOINTS];
} CxiUsbConfiguration;
typedef enum { CXI_USB_DESC_OK=0, CXI_USB_DESC_NULL, CXI_USB_DESC_SHORT, CXI_USB_DESC_BAD_CONFIG, CXI_USB_DESC_BAD_LENGTH, CXI_USB_DESC_TOO_MANY } CxiUsbDescResult;
/* Analyse un buffer deja lu; aucune transaction USB n'est declenchee ici. */
static inline CxiUsbDescResult cxi_usb_parse_configuration(const uint8_t *b, size_t n, CxiUsbConfiguration *out) {
    size_t off,total; int current=-1;
    if (!b || !out) return CXI_USB_DESC_NULL;
    if (n<9U) return CXI_USB_DESC_SHORT;
    if (b[0]<9U || b[1]!=2U) return CXI_USB_DESC_BAD_CONFIG;
    total=(size_t)b[2]|((size_t)b[3]<<8);
    if (total<9U || total>n || b[0]>total) return CXI_USB_DESC_BAD_LENGTH;
    out->total_length=(uint16_t)total; out->configuration_value=b[5];
    out->interface_count=0; out->endpoint_count=0;
    for(off=b[0];off<total;) {
        uint8_t len,type;
        if(total-off<2U) return CXI_USB_DESC_BAD_LENGTH;
        len=b[off]; type=b[off+1U];
        if(len<2U || (size_t)len>total-off) return CXI_USB_DESC_BAD_LENGTH;
        if(type==4U) {
            CxiUsbInterface *it;
            if(len<9U) return CXI_USB_DESC_BAD_LENGTH;
            if(out->interface_count>=CXI_USB_MAX_INTERFACES) return CXI_USB_DESC_TOO_MANY;
            current=(int)out->interface_count++;
            it=&out->interfaces[current];
            it->number=b[off+2U]; it->alternate=b[off+3U];
            it->endpoint_count=0; /* Compte les endpoints effectivement parses. */
            it->klass=b[off+5U]; it->subclass=b[off+6U]; it->protocol=b[off+7U];
        } else if(type==5U) {
            CxiUsbEndpoint *ep;
            if(len<7U || current<0) return CXI_USB_DESC_BAD_LENGTH;
            if(out->endpoint_count>=CXI_USB_MAX_ENDPOINTS) return CXI_USB_DESC_TOO_MANY;
            ep=&out->endpoints[out->endpoint_count++];
            ep->address=b[off+2U]; ep->attributes=b[off+3U];
            ep->max_packet=(uint16_t)b[off+4U]|((uint16_t)b[off+5U]<<8);
            ep->interval=b[off+6U]; ep->interface_number=out->interfaces[current].number;
            out->interfaces[current].endpoint_count++;
        }
        off+=len;
    }
    return CXI_USB_DESC_OK;
}
#endif
