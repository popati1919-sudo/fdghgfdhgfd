# fdghgfdhgfd

## RTL8821CU - etat du developpement

Le coeur logiciel de selection d'interface est dans `src/rtl8821cu.h`.
Il valide l'identite finale `0BDA:C820`, puis selectionne l'interface
vendor-specific avec Bulk IN et Bulk OUT. Il memorise les sorties Bulk
supplementaires et Interrupt IN. Les tests hote sont dans
`tests/test_rtl8821cu.c` et sont lances par `make test`.

### Point bloquant: mode USB initial

La valeur `0BDA:1A2B` observee sur la TI-Nspire CX est coherente avec le
mode d'emulation CD-ROM/installation de certains dongles Realtek. La
documentation de pilotes RTL8821CU sous Linux decrit une transition par
ejection SCSI / `usb_modeswitch -K`, apres laquelle l'adaptateur se
reenumere sous un identifiant reseau tel que `0BDA:C820`.

**Cela n'autorise pas a envoyer cette commande depuis la calculatrice.**
Il faut d'abord verifier que la pile USB hote Ndless expose les primitives
necessaires a une commande SCSI Mass Storage Bulk-Only Transport et que
l'interface de stockage est bien presente dans le mode `1A2B`. Aucune
commande de changement de mode ne doit etre envoyee tant que ces points
ne sont pas verifies.

### Prochaine etape technique

1. Obtenir le rapport USBTreeView complet lorsque le dongle est en
   `0BDA:1A2B` (configuration, interfaces, classe Mass Storage,
   endpoints Bulk IN/OUT).
2. Identifier dans le SDK Ndless les fonctions USB hote documentees et
   verifier leur capacite a enumerer et piloter une interface Mass Storage.
3. Implementer d'abord le decodeur/validateur de la sequence SCSI eject
   en tests PC; ne pas emettre de transfert sur la calculatrice.
4. Seulement apres validation de l'API et des descripteurs, preparer un
   test de changement de mode controle, avec abandon securise si le
   peripherique ou l'interface attendue n'est pas present.

### Limites actuelles

- Les endpoints `0x84`, `0x05`, `0x06`, `0x08`, `0x87` proviennent
  de l'extrait Windows du mode `0BDA:C820`, interface MI_02.
- Ils ne sont pas encore lus directement sur la TI-Nspire CX.
- Le coeur actuel est une couche de selection/test, pas un pilote Wi-Fi.
- Aucun firmware RTL8821CU n'est charge et aucun scan radio n'est effectue.
