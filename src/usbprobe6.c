/* CX Internet - Probe 6: ecran de liste Wi-Fi en attente du pilote.
 * Ne pretend pas effectuer un scan radio. Aucun pilote USB resident n'est installe.
 */
#include <os.h>
#include <libndls.h>

#define TITLE "CX Internet - Reseaux Wi-Fi"

int main(void)
{
    show_msgbox(TITLE,
        "RESEAUX WI-FI\n"
        "--------------------------------\n"
        "Liste des reseaux: indisponible\n\n"
        "Etat du pilote: RTL8821CU non disponible\n"
        "Scan radio: non demarre\n\n"
        "La liste affichera lorsque le pilote sera pret:\n"
        "SSID - Signal - Securite\n\n"
        "Aucun faux reseau n'est affiche.\n"
        "Aucune commande USB n'est envoyee.");
    return 0;
}
