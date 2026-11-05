/* CX Internet - installateur du pilote Wi-Fi RTL8821CU.
 * Etape initiale: verifie les prerequis Ndless sans simuler le chargement.
 * Aucun acces aux registres USB, aucune transaction USB, aucun firmware invente.
 */
#include <os.h>
#include <libndls.h>
#include <usbdi.h>
#include <syscall-decls.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TITLE "CX Internet - Installateur Wi-Fi"

static int has_usb_core(void)
{
    return nl_hassyscall(usb_register_driver) &&
           nl_hassyscall(device_get_ivars);
}

int main(void)
{
    char msg[512];
    int core = has_usb_core();

    snprintf(msg, sizeof(msg),
        "INSTALLATION DU PILOTE RTL8821CU\n"
        "--------------------------------\n"
        "Etape 1/4 : environnement Ndless : %s\n"
        "Etape 2/4 : pilote radio RTL8821CU : NON DISPONIBLE\n"
        "Etape 3/4 : firmware Realtek : NON INSTALLE\n"
        "Etape 4/4 : radio Wi-Fi : NON DEMARREE\n\n"
        "%s\n\n"
        "Cet installateur ne pretend pas avoir installe le pilote. "
        "Le pilote natif et le firmware doivent etre portes et testes "
        "pour le noyau USB de la TI-Nspire avant toute activation. "
        "Aucune commande USB n'a ete envoyee.",
        core ? "PRESENT" : "INCOMPLET",
        core ? "Prerequis systeme detectes." :
               "Fonctions USB Ndless requises indisponibles.");

    show_msgbox(TITLE, msg);
    return 0;
}
