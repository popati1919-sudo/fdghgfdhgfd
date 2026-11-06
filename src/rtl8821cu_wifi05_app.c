/* RTL8821CU Wi-Fi interface 0.5 - TI-Nspire CX / Ndless.
 * Interactive status/menu shell. No fabricated SSIDs and no unsafe USB I/O.
 * Scanning/connection remain blocked until a real RTL8821CU device handle,
 * firmware loader and command/event transport are implemented and validated.
 */
#include <os.h>
#include <libndls.h>
#include <stdio.h>
#include <string.h>
#include "rtl8821cu_core.h"

static void show_status(void)
{
    char s[700];
    snprintf(s, sizeof(s),
        "WI-FI  |  RTL8821CU\n"
        "------------------------------\n"
        "Adaptateur cible : 0BDA:C820\n"
        "Interface Wi-Fi : classe FF\n"
        "USB sur calculatrice : non confirme\n"
        "Identifiant observe : 0BDA:1A2B\n\n"
        "PILOTE : non initialise\n"
        "Firmware : non charge\n"
        "Radio : arretee\n"
        "Reseaux detectes : aucun resultat\n"
        "Connexion : indisponible\n\n"
        "Aucune fausse liste de reseaux n'est affichee.");
    show_msgbox("Etat Wi-Fi", s);
}

static void scan_action(void)
{
    show_msgbox("Recherche Wi-Fi",
        "SCAN IMPOSSIBLE POUR LE MOMENT\n\n"
        "Le pilote RTL8821CU n'a pas encore initialise la radio.\n"
        "La calculatrice n'a pas confirme le PID Wi-Fi 0BDA:C820 "
        "et aucun transfert USB valide n'a ete etabli.\n\n"
        "Aucun scan n'a ete envoye. Aucun reseau fictif n'est ajoute.");
}

static void connect_action(void)
{
    show_msgbox("Connexion Wi-Fi",
        "CONNEXION NON DISPONIBLE\n\n"
        "Il faut d'abord detecter de vrais SSID, choisir un reseau "
        "et initialiser le firmware RTL8821CU.\n\n"
        "Aucun mot de passe n'est demande ou stocke tant que "
        "le canal de communication du pilote n'est pas operationnel.");
}

int main(void)
{
    int choice;
    show_msgbox("Wi-Fi RTL8821CU",
        "Interface Wi-Fi experimentale pour TI-Nspire CX.\n\n"
        "Cette version fournit un vrai menu interactif et un etat "
        "materiel honnete. Le scan et la connexion sont verrouilles "
        "jusqu'a validation du pilote USB.");
    for (;;) {
        choice = show_msgbox_2b("Wi-Fi RTL8821CU",
            "MENU\n\n"
            "1) Etat de l'adaptateur\n"
            "2) Rechercher des reseaux\n"
            "3) Se connecter a un reseau\n\n"
            "Choisir Action pour ouvrir les fonctions.",
            "Action", "Quitter");
        if (choice != 1) break;
        /* Deux boutons seulement sont disponibles via libndls.
         * Le sous-menu alterne les fonctions sans simuler un scan. */
        choice = show_msgbox_2b("Fonctions Wi-Fi",
            "Choisis une fonction :\n\n"
            "ETAT : diagnostic adaptateur\n"
            "SCAN : recherche radio reelle\n"
            "CONNEXION : joindre un SSID",
            "Etat", "Suite");
        if (choice == 1) {
            show_status();
            continue;
        }
        choice = show_msgbox_2b("Fonctions Wi-Fi",
            "Ouvrir la recherche radio ?\n\n"
            "Le pilote n'est pas encore operationnel.",
            "Scanner", "Connexion");
        if (choice == 1) scan_action();
        else connect_action();
    }
    return 0;
}
