/* RTL8821CU Mode Switch 1.0
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

#define RTL_VID 0x0bda
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
static volatile int g_last_status=-1;
static rtl10_softc *g_sc;

static int rtl10_match(device_t self) {
    struct usb_attach_arg *uaa=(struct usb_attach_arg *)device_get_ivars(self);
    usb_interface_descriptor_t *id;
    if(!uaa || !uaa->iface || uaa->vendor!=RTL_VID || uaa->product!=RTL_PID_CDROM)
        return UMATCH_NONE;
    id=usbd_get_interface_descriptor(uaa->iface);
    if(!id || id->bInterfaceClass!=0x08 || id->bInterfaceSubClass!=0x06 ||
       id->bInterfaceProtocol!=0x50) return UMATCH_NONE;
    ++g_match;
    return UMATCH_IFACECLASS_IFACESUBCLASS_IFACEPROTO;
}
static int rtl10_attach(device_t self) {
    struct usb_attach_arg *uaa=(struct usb_attach_arg *)device_get_ivars(self);
    rtl10_softc *sc=(rtl10_softc *)device_get_softc(self);
    usb_interface_descriptor_t *id;
    uint8_t count=0, i;
    int found_in=0,found_out=0;
    if(!uaa || !uaa->iface || !sc || uaa->vendor!=RTL_VID ||
       uaa->product!=RTL_PID_CDROM) return -1;
    id=usbd_get_interface_descriptor(uaa->iface);
    if(!id || id->bInterfaceClass!=0x08 || id->bInterfaceSubClass!=0x06 ||
       id->bInterfaceProtocol!=0x50) return -1;
    memset(sc,0,sizeof(*sc));
    if(usbd_endpoint_count(uaa->iface,&count)!=USBD_NORMAL_COMPLETION) return -1;
    for(i=0;i<count;i++) {
        usb_endpoint_descriptor_t *ep=usbd_get_endpoint_descriptor(uaa->iface,i);
        if(!ep || (ep->bmAttributes & UE_XFERTYPE)!=UE_BULK) continue;
        if((ep->bEndpointAddress & UE_DIR_IN) && !found_in) {
            sc->ep_in=ep->bEndpointAddress; sc->mps_in=UGETW(ep->wMaxPacketSize); found_in=1;
        } else if(!(ep->bEndpointAddress & UE_DIR_IN) && !found_out) {
            sc->ep_out=ep->bEndpointAddress; sc->mps_out=UGETW(ep->wMaxPacketSize); found_out=1;
        }
    }
    if(!found_in || !found_out) return -1;
    if(usbd_interface2device_handle(uaa->iface,&sc->dev)!=USBD_NORMAL_COMPLETION) return -1;
    sc->iface=uaa->iface; sc->attached=1; g_sc=sc; g_attached=1;
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
                    "Attends la disparition USB, puis rebranche l'antenne.\n"
                    "Le PID C820 devra etre confirme separement.");
    return 0;
}
int main(void) {
    int reg,choice;
    char report[700];
    reg=usb_register_driver(2,rtl10_methods,"rtl8821cu-ms10",0,sizeof(rtl10_softc));
    nl_set_resident();
    for(;;) {
        snprintf(report,sizeof(report),
          "RTL8821CU MODE SWITCH 1.0\n\n"
          "Enregistrement Ndless : %s (%d)\n"
          "Match interface CD-ROM 08/06/50 : %d\n"
          "Adaptateur compatible attache : %s\n"
          "Endpoints Bulk IN/OUT : %s\n\n"
          "Cette version envoie SCSI EJECT uniquement apres confirmation.",
          reg==0?"OK":"ECHEC",reg,(int)g_match,g_attached?"OUI":"NON",
          (g_attached && g_sc)?"DETECTES":"NON DISPONIBLES");
        show_msgbox("RTL8821CU Mode Switch 1.0",report);
        if(!g_attached || !g_sc) {
            choice=show_msgbox_2b("MODE SWITCH 1.0",
              "Branche l'antenne en mode CD-ROM 0BDA:1A2B puis actualise.",
              "Actualiser","Quitter");
            if(choice!=1) break;
            continue;
        }
        choice=show_msgbox_2b("CONFIRMATION SCSI EJECT",
          "Envoyer START STOP UNIT / LOEJ=1 au lecteur CD-ROM detecte ?",
          "EJECTER","Annuler");
        if(choice!=1) break;
        {
            int result=do_eject(g_sc,report,sizeof(report));
            g_last_status=result;
            show_msgbox("Resultat SCSI EJECT",report);
        }
        choice=show_msgbox_2b("MODE SWITCH 1.0",
          "Actualiser apres disparition/reconnexion USB ?",
          "Actualiser","Quitter");
        if(choice!=1) break;
    }
    _exit(0);
}
