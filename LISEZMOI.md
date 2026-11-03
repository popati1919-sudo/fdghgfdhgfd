# CX Internet - Etape 1 (sonde) et etape 1b (VID:PID)

Ce programme **ne modifie rien**. Il lit l'etat du controleur USB de ta
TI-Nspire CX et l'affiche, pour repondre a deux questions :

1. La calculatrice voit-elle le cable OTG (mode hote) ?
2. Voit-elle un appareil branche (ton antenne BT+AC600, Realtek 8821CU) ?

Les adresses viennent du projet Nspire-MediaPlayer-Ndless (GPLv3), deja
valides sur CX et CX II. Ce projet est donc aussi sous GPLv3.

**Non teste sur calculatrice** : le code a ete verifie sur PC (tests de
decodage + verification de syntaxe), mais pas compile pour ARM ni execute sur
une CX. Tu es le premier a l'essayer. Ton retour est la suite du projet.

## Compiler

Option A - GitHub Actions (le plus simple, pas de compilateur a installer)
1. Cree un depot GitHub prive et envoies-y ce dossier.
2. Onglet Actions > "Build USB probe" > Run workflow.
3. La premiere fois, la construction du compilateur ARM prend longtemps
   (30 a 90 min). Les fois suivantes, c'est rapide (cache).
4. Telecharge l'artefact `usbprobe` : il contient `usbprobe.tns`.

Option B - Windows avec WSL2 (Ubuntu)
```bash
sudo apt update
sudo apt install -y build-essential bison flex libboost-program-options-dev \
  libgmp-dev libmpc-dev libmpfr-dev libtinfo-dev python3-dev texinfo xz-utils zlib1g-dev git
git clone --recurse-submodules https://github.com/ndless-nspire/Ndless.git
cd Ndless/ndless-sdk/toolchain && ./build_toolchain.sh   # long: 30 a 90 min
cd .. && export PATH=$PWD/toolchain/install/bin:$PWD/bin:$PATH
make build-libsyscalls build-libndls build-thirdparty build-tools
cd /chemin/vers/cx-internet && make test && make
```
(adapte `/chemin/vers/cx-internet` ; sous WSL2, tes fichiers Windows sont dans `/mnt/c/...`)

## Utiliser

1. Copie `usbprobe.tns` sur la calculatrice (logiciel TI-Nspire pour ordinateur).
2. Lance-le depuis Documents (Ndless doit etre actif).
3. Branche le cable OTG mini-USB vers USB-A, puis l'antenne, **avant** d'appuyer sur Entree.
4. Note l'ecran et envoie-le moi. "Relire" relit l'etat sans rien modifier.

## Lire le resultat

| Message | Sens | Suite |
|---|---|---|
| Cable OTG NON detecte | Mode peripherique | Mauvais cable (il faut un vrai OTG) |
| OTG detecte, aucun appareil | Cable vu, antenne non vue | Probable manque de courant: alimentation externe |
| OTG detecte ET appareil | Bon signe | Etape 1b: lire VID:PID |
| Endormi / illisible | Non concluant | Relire, rebrancher |

## Limites

- Cette etape ne lit **pas** l'identifiant USB (VID:PID). Il faut pour cela
  enumerer l'appareil via la pile USB du systeme, comme le fait le lecteur
  media: c'est l'etape 1b, a faire une fois celle-ci validee.
- Rien ici ne gere le Wi-Fi: pas de pilote RTL8821CU, de WPA2, de TCP/IP ni de HTTPS.


---

# Etape 1b : lire le VID:PID (`usbprobe1b.tns`)

Meme methode que le lecteur video (GPLv3): enregistrer un pilote aupres de la
pile USB du systeme. Ici c'est un **espion**: quand le systeme sonde un appareil,
il note son identifiant (usb_attach_arg: vendeur, produit, interface, vitesse)
puis repond "pas pour moi". Il ne prend aucun appareil en charge et n'ecrit dans
aucun registre USB.

## Ce qui est verifie / pas verifie

- Verifie sur PC et sous emulateur ARM (`make test-arm`): le code resident, avec
  une fausse chaine device_t -> usb_attach_arg -> device (cas normal, pointeurs
  invalides, anneau de 4 entrees, aucune ecriture hors du bloc). La syntaxe du
  programme principal est verifiee contre les en-tetes du SDK Ndless.
- **Non verifie sur calculatrice**, et pas compile en .tns ici. Surtout, je n'ai pas pu
  confirmer que l'OS 4.2.0.532 a la meme disposition interne que celle sur
  laquelle le lecteur video est valide (4.5+).
- Garde-fous: si la fonction `device_get_ivars` du systeme n'a pas la forme
  attendue, le programme s'arrete AVANT d'enregistrer quoi que ce soit et affiche
  deux valeurs (envoie-les moi, elles servent de diagnostic).
- Si le programme se plante apres l'enregistrement: retire la pile (ou reset au
  dos) et redemarre; le pilote disparait au redemarrage.

## Utiliser

1. Compile (GitHub Actions: l'artefact `usbprobes` contient les deux .tns) et copie
   `usbprobe1b.tns` sur la calculatrice.
2. Branche le cable OTG, lance le programme, Entree: le pilote espion s'installe.
3. Debranche puis rebranche l'antenne, attends 3 s, choisis "Relire".
4. Note l'ecran. Ne lance pas le programme deux fois sans redemarrer.

## Lire le resultat

| Ecran | Sens |
|---|---|
| `VID:PID 0BDA:C811 ...` | Succes: identifiant lu |
| "installe, mais rien sonde" | Le systeme n'a pas re-sonde: rebrancher l'antenne, sinon manque de courant |
| `lecture arretee (etape N/4)` | Le systeme a sonde mais la structure n'a pas la forme attendue: envoie l'ecran |
| Message "rien n'a ete enregistre" | Garde-fou: envoie les deux valeurs affichees |

Rappel: connaitre le VID:PID ne rend pas le Wi-Fi possible (pas de pilote RTL8821CU,
WPA2, TCP/IP ni TLS sur la CX). Le VID:PID s'obtient aussi en 10 secondes sur un PC.
