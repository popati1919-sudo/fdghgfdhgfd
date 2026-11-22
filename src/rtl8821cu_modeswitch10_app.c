/* RTL8821CU Mode Switch 1.1
 * Real USB Mass-Storage BOT SCSI START STOP UNIT (LOEJ=1).
 * Strictly matches only 0BDA:1A2B, interface 08/06/50, with one Bulk IN
 * and one Bulk OUT endpoint discovered from live descriptors. User-confirmed.
 */
#include <os.h>
#include <libndls.h>
#include <nucleus.h>
#include <usbdi.h>
#include <syscall-decls.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "usb_descriptors.h"

#define RTL_VID 0x0bda
#define RTL_PID_WIFI 0xc820
#define RTL_PID_CDROM 0x1a2b
#define BOT_CBW_LEN 31
#define BOT_CSW_LEN 13
#define BOT_TIMEOUT_MS 4000

typedef struct {
    int attached;
    usbd_device_handle dev;
    usbd_interface_handle iface;
    uint8_t ep_in, ep_out;
    uint16_t mps_in, mps_out;
} rtl10_softc;

static volatile int g_attached, g_detached, g_match;
/* Match callbacks can occur for each interface. Track observations, not devices. */
static volatile unsigned wifi_observations;
static rtl10_softc *g_sc;

static volatile unsigned cd_device_probes;
static volatile int config_seen;

static char config_report[100] = "En attente du CD-ROM";

static void read_configuration(usbd_device_handle dev) {
    usb_device_request_t request = {0};
    uint8_t header[USB_CONFIG_DESCRIPTOR_SIZE] = {0};
    int received = 0;
    usbd_status status;
    if (!dev) {
        snprintf(config_report, sizeof(config_report), "Handle USB absent");
        return;
    }
    request.bmRequestType = UT_READ_DEVICE;
    request.bRequest = UR_GET_DESCRIPTOR;
    USETW(request.wValue, UDESC_CONFIG << 8);
    USETW(request.wLength, sizeof(header));
    status = usbd_do_request_flags(dev, &request, header, 0, &received, 1000);
    if (status != USBD_NORMAL_COMPLETION || received != (int)sizeof(header)) {
        snprintf(config_report, sizeof(config_report), "Statut USB %d, recu %d/9", (int)status, received);
        return;
    }
    unsigned total = header[2] | ((unsigned)header[3] << 8);
    if (header[0] != sizeof(header) || header[1] != UDESC_CONFIG ||
        total < sizeof(header) || !header[5]) {
        snprintf(config_report, sizeof(config_report), "Entete de configuration invalide");
        return;
    }
    uint8_t raw[256];
    CxiUsbConfiguration cfg;
    if (total > sizeof(raw)) {
        snprintf(config_report, sizeof(config_report), "Configuration trop longue : %u", total);
        return;
    }
    USETW(request.wLength, total);
    received = 0;
    status = usbd_do_request_flags(dev, &request, raw, 0, &received, 1000);
    if (status != USBD_NORMAL_COMPLETION || received != (int)total) {
        snprintf(config_report, sizeof(config_report), "Lecture complete : USB %d, %d/%u octets",
                 (int)status, received, total);
        return;
    }
    if (memcmp(raw, header, sizeof(header)) ||
        cxi_usb_parse_configuration(raw, total, &cfg) != CXI_USB_DESC_OK ||
        cfg.interface_count != 1) {
        snprintf(config_report, sizeof(config_report), "Configuration invalide ou plusieurs interfaces");
        return;
    }
    CxiUsbInterface *iface = &cfg.interfaces[0];
    unsigned in = 0, out = 0;
    for (unsigned i = 0; i < cfg.endpoint_count; ++i) {
        CxiUsbEndpoint *ep = &cfg.endpoints[i];
        if ((ep->attributes & 3) != UE_BULK) continue;
        if (ep->address & UE_DIR_IN) in = ep->address;
        else out = ep->address;
    }
    snprintf(config_report, sizeof(config_report),
             "Interface %u : %02X/%02X/%02X\nBulk annonce IN=%02X OUT=%02X",
             (unsigned)iface->number, (unsigned)iface->klass, (unsigned)iface->subclass,
             (unsigned)iface->protocol, in, out);
}

static volatile unsigned interface_probes;
static volatile unsigned storage_probes;

/* An interface probe can omit the identifiers carried by the device probe. */
static int storage_identity(struct usb_attach_arg *uaa) {
    usb_device_request_t req = {0};
    usb_device_descriptor_t desc = {0};
    usbd_device_handle dev = NULL;
    int received = 0;
    if (uaa->vendor == RTL_VID && uaa->product == RTL_PID_CDROM) return 1;
    if (!uaa->iface || usbd_interface2device_handle(uaa->iface, &dev) != USBD_NORMAL_COMPLETION || !dev)
        return 0;
    req.bmRequestType = UT_READ_DEVICE;
    req.bRequest = UR_GET_DESCRIPTOR;
    USETW(req.wValue, UDESC_DEVICE << 8);
    USETW(req.wLength, sizeof(desc));
    if (usbd_do_request_flags(dev, &req, &desc, 0, &received, 1000) != USBD_NORMAL_COMPLETION ||
        received != (int)sizeof(desc) || desc.bLength != sizeof(desc) || desc.bDescriptorType != UDESC_DEVICE)
        return 0;
    return UGETW(desc.idVendor) == RTL_VID && UGETW(desc.idProduct) == RTL_PID_CDROM;
}

static usbd_interface_handle find_storage(struct usb_attach_arg *uaa) {
    uint8_t count = 1;
    unsigned i;
    if (!uaa || (!uaa->iface && (uaa->vendor != RTL_VID || uaa->product != RTL_PID_CDROM)))
        return NULL;
    if (!uaa->iface) {
        if (!uaa->device || !usbd_get_config_descriptor(uaa->device))
            return NULL;
        config_seen = 1;
        if (usbd_interface_count(uaa->device, &count) != USBD_NORMAL_COMPLETION)
            return NULL;
    }
    for (i = 0; i < count; ++i) {
        usbd_interface_handle iface = uaa->iface;
        usb_interface_descriptor_t *id;
        if (!iface && usbd_device2interface_handle(uaa->device, i, &iface) != USBD_NORMAL_COMPLETION)
            continue;
        if (!iface) continue;
        id = usbd_get_interface_descriptor(iface);
        if (id && id->bInterfaceClass == 8 && id->bInterfaceSubClass == 6 &&
            id->bInterfaceProtocol == 0x50) {
            if (storage_identity(uaa)) return iface;
        }
    }
    return NULL;
}
static int rtl10_match(device_t self) {
    struct usb_attach_arg *uaa=(struct usb_attach_arg *)device_get_ivars(self);
    if (uaa && uaa->iface) {
        usb_interface_descriptor_t *id = usbd_get_interface_descriptor(uaa->iface);
        ++interface_probes;
        if (id && id->bInterfaceClass == 8 && id->bInterfaceSubClass == 6 &&
            id->bInterfaceProtocol == 0x50) ++storage_probes;
    }
    if (uaa && uaa->vendor == RTL_VID && uaa->product == RTL_PID_WIFI) {
        ++wifi_observations;
        return UMATCH_NONE;
    }
    if (uaa && uaa->vendor == RTL_VID && uaa->product == RTL_PID_CDROM && !uaa->iface) {
        ++cd_device_probes;
        if (!uaa->device || !usbd_get_config_descriptor(uaa->device))
            read_configuration(uaa->device);
    }
    if (!find_storage(uaa)) return UMATCH_NONE;
    ++g_match;
    return uaa->iface ? UMATCH_IFACECLASS_IFACESUBCLASS_IFACEPROTO : UMATCH_VENDOR_PRODUCT;
}
static int rtl10_attach(device_t self) {
    struct usb_attach_arg *uaa=(struct usb_attach_arg *)device_get_ivars(self);
    rtl10_softc *sc=(rtl10_softc *)device_get_softc(self);
    usbd_interface_handle iface = find_storage(uaa);
    uint8_t count=0, i;
    int found_in=0,found_out=0;
    if (!iface || !sc) return -1;
    memset(sc,0,sizeof(*sc));
    if(usbd_endpoint_count(iface,&count)!=USBD_NORMAL_COMPLETION) return -1;
    for(i=0;i<count;i++) {
        usb_endpoint_descriptor_t *ep=usbd_get_endpoint_descriptor(iface,i);
        if(!ep || (ep->bmAttributes & UE_XFERTYPE)!=UE_BULK) continue;
        if((ep->bEndpointAddress & UE_DIR_IN) && !found_in) {
            sc->ep_in=ep->bEndpointAddress; sc->mps_in=UGETW(ep->wMaxPacketSize); found_in=1;
        } else if(!(ep->bEndpointAddress & UE_DIR_IN) && !found_out) {
            sc->ep_out=ep->bEndpointAddress; sc->mps_out=UGETW(ep->wMaxPacketSize); found_out=1;
        }
    }
    if(!found_in || !found_out) return -1;
    if(usbd_interface2device_handle(iface,&sc->dev)!=USBD_NORMAL_COMPLETION) return -1;
    sc->iface=iface; sc->attached=1; g_sc=sc; g_attached=1;
    return 0;
}
static int rtl10_detach(device_t self) {
    rtl10_softc *sc=(rtl10_softc *)device_get_softc(self);
    if(sc) sc->attached=0;
    g_sc=0; g_attached=0; ++g_detached;
    return 0;
}
static int (*rtl10_methods[])(device_t)={rtl10_match,rtl10_attach,rtl10_detach,NULL};

static usbd_status bulk_xfer(usbd_device_handle dev, usbd_pipe_handle pipe,
                             void *buf, uint32_t len) {
    usbd_xfer_handle x=usbd_alloc_xfer(dev);
    usbd_status st;
    if(!x) return USBD_NOMEM;
    usbd_setup_xfer(x,pipe,0,buf,len,USBD_SYNCHRONOUS,BOT_TIMEOUT_MS,0);
    st=usbd_sync_transfer(x);
    usbd_free_xfer(x);
    return st;
}
static void put_le32(uint8_t *p,uint32_t v) {
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
static int do_eject(rtl10_softc *sc,char *out,size_t cap) {
    usbd_pipe_handle pin=0,pout=0;
    uint8_t cbw[BOT_CBW_LEN],csw[BOT_CSW_LEN];
    usbd_status st;
    uint32_t tag=0x1021c0de;
    if(!sc || !sc->attached || !sc->dev || !sc->iface) {
        snprintf(out,cap,"Adaptateur CD-ROM non attache.");
        return -1;
    }
    if(usbd_open_pipe(sc->iface,sc->ep_out,0,&pout)!=USBD_NORMAL_COMPLETION ||
       usbd_open_pipe(sc->iface,sc->ep_in,0,&pin)!=USBD_NORMAL_COMPLETION) {
        if(pout) usbd_close_pipe(pout);
        snprintf(out,cap,"Impossible d'ouvrir les pipes Bulk.");
        return -2;
    }
    memset(cbw,0,sizeof(cbw)); memset(csw,0,sizeof(csw));
    put_le32(cbw,0x43425355U); put_le32(cbw+4,tag);
    cbw[14]=6; cbw[15]=0x1b; cbw[19]=0x02;
    st=bulk_xfer(sc->dev,pout,cbw,sizeof(cbw));
    if(st!=USBD_NORMAL_COMPLETION) {
        snprintf(out,cap,"Echec envoi CBW : statut USB %d.",(int)st);
        usbd_close_pipe(pin);usbd_close_pipe(pout);return -3;
    }
    st=bulk_xfer(sc->dev,pin,csw,sizeof(csw));
    if(st!=USBD_NORMAL_COMPLETION) {
        snprintf(out,cap,"CBW envoye; lecture CSW echouee : statut %d. Debranche/rebranche si necessaire.",(int)st);
        usbd_close_pipe(pin);usbd_close_pipe(pout);return -4;
    }
    usbd_close_pipe(pin);usbd_close_pipe(pout);
    if(csw[0]!='U'||csw[1]!='S'||csw[2]!='B'||csw[3]!='S' ||
       csw[4]!=(uint8_t)tag || csw[5]!=(uint8_t)(tag>>8) ||
       csw[6]!=(uint8_t)(tag>>16) || csw[7]!=(uint8_t)(tag>>24)) {
        snprintf(out,cap,"CSW invalide (signature/tag). Reponse BOT non valide.");
        return -5;
    }
    if(csw[12]!=0) {
        snprintf(out,cap,"Peripherique a refuse SCSI EJECT (CSW status=%u).",csw[12]);
        return -6;
    }
    snprintf(out,cap,"Commande SCSI EJECT acceptee (CSW valide).\n"
                    "Laisse l'antenne branchee et choisis Actualiser pour verifier le PID C820.");
    return 0;
}
int main(void) {
    int reg,choice;
    char report[700];
    unsigned before_eject = 0;
    int eject_attempted = 0;
    reg=usb_register_driver(2,rtl10_methods,"rtl8821cu-ms10",0,sizeof(rtl10_softc));
    nl_set_resident();
    for(;;) {
        snprintf(report,sizeof(report),
          "Sondes interface / stockage : %u / %u\nDescripteur USB lu : %s\n\n"
          "Enregistrement Ndless : %s (%d)\n"
          "Sondes CD-ROM sans iface : %u\n"
          "Configuration vue par ces sondes : %s\n"
          "Match interface CD-ROM 08/06/50 : %d\n"
          "Adaptateur compatible attache : %s\n"
          "Endpoints Bulk IN/OUT : %s\n\n"
          "PID Wi-Fi 0BDA:C820 vu %s : %s",
          (unsigned)interface_probes,(unsigned)storage_probes,config_report,reg==0?"OK":"ECHEC",reg,(unsigned)cd_device_probes,config_seen?"OUI":"PAS ENCORE",(int)g_match,g_attached?"OUI":"NON",
          (g_attached && g_sc)?"DETECTES":"NON DISPONIBLES",
          eject_attempted ? "depuis la tentative EJECT" : "depuis le lancement",
          wifi_observations != before_eject ? "OUI" : "PAS ENCORE");
        show_msgbox("RTL8821CU Mode Switch 1.1",report);
        if(!g_attached || !g_sc) {
            choice=show_msgbox_2b("MODE SWITCH 1.1",
              "Actualise pour relire les observations USB.",
              "Actualiser","Quitter");
            if(choice!=1) break;
            continue;
        }
        choice=show_msgbox_2b("CONFIRMATION SCSI EJECT",
          "Envoyer START STOP UNIT / LOEJ=1 au lecteur CD-ROM detecte ?",
          "EJECTER","Annuler");
        if(choice!=1) break;
        {
            before_eject = wifi_observations;
            eject_attempted = 1;
            do_eject(g_sc,report,sizeof(report));
            show_msgbox("Resultat SCSI EJECT",report);
        }
        choice=show_msgbox_2b("MODE SWITCH 1.1",
          "Actualiser apres disparition/reconnexion USB ?",
          "Actualiser","Quitter");
        if(choice!=1) break;
    }
    _exit(0);
}
