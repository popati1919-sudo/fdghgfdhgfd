/* CX Internet - pilote espion USB (etape 1b). Licence GPLv3.
 * Code resident, position-independent: copie en memoire OS qui n'est jamais
 * liberee, car le systeme peut rappeler ces fonctions apres la fin du programme.
 * Il ne depend donc d'aucun symbole du programme.
 *
 * match(r0 = device_t): note ce que le systeme sonde dans le contexte, puis
 * repond UMATCH_NONE (0): l'appareil n'est jamais pris en charge, rien ne change.
 * Aucune ecriture hors du contexte; aucune lecture hors de 0x10000000-0x13bfffff
 * (RAM de la CX), alignee sur 4 octets.
 *
 * Contexte (cf. usb_spy.h): [0]=compteur, [1]=decalage ivars, puis anneau de
 * 4 entrees de 14 mots a l'offset 64. */
.syntax unified
.arm
.text
.balign 4
.global cxi_spy8_start, cxi_spy8_end, cxi_spy8_context
.global cxi_spy8_match, cxi_spy8_nop
cxi_spy8_start:
cxi_spy8_match:
    push {r4-r8,lr}
    ldr r4, cxi_spy8_context
    ldr r5, [r4]                /* ancien compteur */
    and r6, r5, #3
    mov r8, r6, lsl #3          /* 8 x index */
    sub r6, r8, r6              /* 7 x index */
    mov r6, r6, lsl #3          /* 56 octets par entree */
    add r7, r4, #64
    add r7, r7, r6              /* r7 = entree de l'anneau */
    add r5, r5, #1
    str r5, [r4]
    mov r8, #0
    add r3, r7, #4
    mov r2, #13
0:  str r8, [r3], #4            /* efface les mots 1..11 */
    subs r2, r2, #1
    bne 0b
    str r0, [r7]                /* e[0] = device_t */
    mov r8, #1
    str r8, [r7, #44]           /* etape atteinte = 1 */
    tst r0, #3
    bne 9f
    cmp r0, #0x10000000
    blo 9f
    cmp r0, #0x13c00000
    bhs 9f
    ldr r5, [r4, #4]            /* decalage de device_get_ivars */
    ldr r1, [r0, r5]            /* a = struct usb_attach_arg * */
    str r1, [r7, #4]            /* e[1] = a */
    mov r8, #2
    str r8, [r7, #44]
    tst r1, #3
    bne 9f
    cmp r1, #0x10000000
    blo 9f
    cmp r1, #0x13c00000
    bhs 9f
    mov r2, #8
    add r3, r7, #8
1:  ldr r5, [r1], #4            /* copie 8 mots de usb_attach_arg */
    str r5, [r3], #4
    subs r2, r2, #1
    bne 1b
    ldr r5, [r1, #4]           /* uaa.ifaces (base + 36) */
    str r5, [r7, #48]
    ldr r5, [r1, #8]           /* uaa.nifaces (base + 40) */
    str r5, [r7, #52]
    mov r8, #3
    str r8, [r7, #44]
    ldr r1, [r7, #32]           /* uaa.device */
    tst r1, #3
    bne 9f
    cmp r1, #0x10000000
    blo 9f
    cmp r1, #0x13c00000
    bhs 9f
    ldr r5, [r1, #8]            /* adresse, config, profondeur, vitesse */
    str r5, [r7, #40]
    mov r8, #4
    str r8, [r7, #44]
9:  mov r0, #0                  /* UMATCH_NONE */
    pop {r4-r8,lr}
    bx lr
cxi_spy8_nop:
    mov r0, #0
    bx lr
.balign 4
cxi_spy8_context:
    .word 0
cxi_spy8_end:
