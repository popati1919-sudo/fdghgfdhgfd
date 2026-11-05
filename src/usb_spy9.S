/* CX Internet Probe 9: snapshot brut de usb_attach_arg.
 * GPLv3. Ne lit aucun pointeur imbrique et n'envoie aucune requete USB.
 * Copie uniquement les 11 champs scalaires/pointeurs du struct connu (44 octets).
 */
.syntax unified
.arm
.text
.balign 4
.global cxi_spy9_start, cxi_spy9_end, cxi_spy9_context
.global cxi_spy9_match, cxi_spy9_nop
cxi_spy9_start:
cxi_spy9_match:
    push {r4-r8,lr}
    ldr r4, cxi_spy9_context
    ldr r5, [r4]
    and r6, r5, #3
    add r7, r4, #64
    add r7, r7, r6, lsl #6
    add r5, r5, #1
    str r5, [r4]
    mov r8, #0
    add r3, r7, #4
    mov r2, #13
0:  str r8, [r3], #4
    subs r2, r2, #1
    bne 0b
    str r0, [r7]
    mov r8, #1
    str r8, [r7, #52]           /* stage = 1 */
    tst r0, #3
    bne 9f
    cmp r0, #0x10000000
    blo 9f
    cmp r0, #0x13c00000
    bhs 9f
    ldr r5, [r4, #4]
    ldr r1, [r0, r5]            /* device_get_ivars(device) */
    str r1, [r7, #4]
    mov r8, #2
    str r8, [r7, #52]
    tst r1, #3
    bne 9f
    cmp r1, #0x10000000
    blo 9f
    cmp r1, #0x13bfffd4          /* leave 44 bytes inside checked RAM window */
    bhi 9f
    add r3, r7, #8
    mov r2, #11
1:  ldr r5, [r1], #4
    str r5, [r3], #4
    subs r2, r2, #1
    bne 1b
    mov r8, #4
    str r8, [r7, #52]
9:  mov r0, #0                  /* UMATCH_NONE */
    pop {r4-r8,lr}
    bx lr
cxi_spy9_nop:
    mov r0, #0
    bx lr
.balign 4
cxi_spy9_context:
    .word 0
cxi_spy9_end:
