/* RTL8821CU USB device-level diagnostic 0.8 - TI-Nspire CX / Ndless.
 * Match the complete USB device by VID/PID. No interface handle assumption.
 * No interface-array dereference, endpoint, pipe, SCSI/BOT, bulk or firmware.
 */
#include <os.h>
#include <libndls.h>
#include <nucleus.h>
#include <usbdi.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define RTL_VID 0x0bda
#define RTL_PID_CDROM 0x1a2b
#define RTL_PID_WIFI 0xc820
typedef struct { int attached; } rtl_diag_sc_t;
static volatile int g_match,g_attach,g_detach,g_null_uaa;
static volatile int g_last_pid=-1,g_last_port=-1,g_last_config=-1,g_last_if=-1;
static volatile int g_cd_seen,g_wifi_seen;
static void record_device(struct usb_attach_arg *uaa) {
 if(!uaa){++g_null_uaa;return;}
 g_last_pid=uaa->product;g_last_port=uaa->port;
 g_last_config=uaa->configno;g_last_if=uaa->ifaceno;
 if(uaa->vendor==RTL_VID&&uaa->product==RTL_PID_CDROM)g_cd_seen=1;
 if(uaa->vendor==RTL_VID&&uaa->product==RTL_PID_WIFI)g_wifi_seen=1;
}
static int rtl_match(device_t self) {
 struct usb_attach_arg *uaa=(struct usb_attach_arg *)device_get_ivars(self);
 if(!uaa){++g_null_uaa;return UMATCH_NONE;}
 if(uaa->vendor!=RTL_VID)return UMATCH_NONE;
 if(uaa->product!=RTL_PID_CDROM&&uaa->product!=RTL_PID_WIFI)return UMATCH_NONE;
 record_device(uaa);++g_match;return UMATCH_VENDOR_PRODUCT;
}
static int rtl_attach(device_t self) {
 struct usb_attach_arg *uaa=(struct usb_attach_arg *)device_get_ivars(self);
 rtl_diag_sc_t *sc=(rtl_diag_sc_t *)device_get_softc(self);
 if(!uaa){++g_null_uaa;return -1;}
 if(sc)sc->attached=1;
 record_device(uaa);++g_attach;return 0;
}
static int rtl_detach(device_t self) {
 rtl_diag_sc_t *sc=(rtl_diag_sc_t *)device_get_softc(self);
 if(sc)sc->attached=0;
 ++g_detach;return 0;
}
static int (*rtl_methods[])(device_t)={rtl_match,rtl_attach,rtl_detach,NULL};
static void show_report(int registration) {
 char report[1000];
 snprintf(report,sizeof(report),
 "RTL8821CU - USB DEVICE DIAGNOSTIC 0.8\n"
 "------------------------------------\n"
 "Enregistrement Ndless : %s (%d)\n"
 "Callbacks match/attach/detach : %d/%d/%d\n"
 "usb_attach_arg NULL : %d\n\n"
 "Dernier PID : %04X\n"
 "Port / config / interface : %d / %d / %d\n\n"
 "Mode CD-ROM 0BDA:1A2B : %s\n"
 "Mode Wi-Fi 0BDA:C820 : %s\n\n"
 "Match au niveau du peripherique complet.\n"
 "Aucun handle iface/ifaces n'est lu ou dereference.\n"
 "Aucun endpoint, pipe, SCSI/BOT, bulk ou firmware.",
 registration==0?"OK":"ECHEC",registration,
 (int)g_match,(int)g_attach,(int)g_detach,(int)g_null_uaa,
 (unsigned)(g_last_pid<0?0:g_last_pid),(int)g_last_port,
 (int)g_last_config,(int)g_last_if,
 g_cd_seen?"OBSERVE":"pas observe",g_wifi_seen?"OBSERVE":"pas observe");
 show_msgbox("RTL8821CU USB Diagnostic 0.8",report);
}
int main(void) {
 int registration,choice;
 registration=usb_register_driver(2,rtl_methods,"rtl8821cu-usbdiag08",0,
                                  (unsigned int)sizeof(rtl_diag_sc_t));
 nl_set_resident();
 for(;;) {
  show_report(registration);
  choice=show_msgbox_2b("RTL8821CU - SUIVI 0.8",
   "Actualiser apres branchement/debranchement ?","Actualiser","Quitter");
  if(choice!=1)_exit(0);
 }
 _exit(0);
}
