/* RTL8821CU Wi-Fi interface 0.6 - TI-Nspire CX / Ndless.
 * Menu exposes ETAT, SCAN and CONNEXION as separate actions.
 * No fabricated SSIDs and no unsafe USB I/O.
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
    show_msgbox("ETAT - Adaptateur Wi-Fi", s);
}

static void scan_action(void)
{
    show_msgbox("SCAN - Reseaux Wi-Fi",
        "RECHERCHE RADIO NON DISPONIBLE\n\n"
        "Le pilote RTL8821CU n'a pas encore initialise la radio.\n"
        "La calculatrice n'a pas confirme le PID Wi-Fi 0BDA:C820 "
        "et aucun transfert USB valide n'a ete etabli.\n\n"
        "Aucun scan radio n'a ete lance.\n"
        "Aucun reseau fictif n'est affiche.");
}

static void connect_action(void)
{
    show_msgbox("CONNEXION - Reseau Wi-Fi",
        "CONNEXION NON DISPONIBLE\n\n"
        "Le scan radio doit d'abord detecter de vrais SSID.\n"
        "Ensuite, le pilote devra initialiser le firmware RTL8821CU "
        "et etablir le canal de communication.\n\n"
        "Aucun mot de passe n'est demande ou stocke pour le moment.");
}

int main(void)
{
    int choice;
    show_msgbox("CX Internet - Wi-Fi",
        "Interface experimentale pour TI-Nspire CX.\n\n"
        "Les trois fonctions sont separees : ETAT, SCAN et CONNEXION.\n"
        "Le scan et la connexion restent bloques tant que le pilote "
        "USB RTL8821CU n'est pas operationnel.");
    for (;;) {
        choice = show_msgbox_2b("FONCTIONS WI-FI",
            "Choisis une fonction :\n\n"
            "ETAT : diagnostic adaptateur\n"
            "SCAN : recherche radio reelle\n"
            "CONNEXION : joindre un SSID\n\n"
            "Etat = diagnostic\n"
            "Suite = Scan / Connexion",
            "Etat", "Suite");
        if (choice != 1) break;
        show_status();
        /* After ETAT, return to the same three-function menu. */
    }
    /* Suite ouvre le sous-menu explicite SCAN / CONNEXION. */
    for (;;) {
        choice = show_msgbox_2b("SCAN / CONNEXION",
            "Choisis une fonction :\n\n"
            "SCAN : rechercher les reseaux Wi-Fi\n"
            "CONNEXION : joindre un SSID\n\n"
            "Scanner = lancer le scan disponible\n"
            "Connexion = options de connexion",
            "Scanner", "Connexion");
        if (choice == 1) {
            scan_action();
            /* Revenir au choix SCAN / CONNEXION. */
            continue;
        }
        connect_action();
        break;
    }
    return 0;
}
