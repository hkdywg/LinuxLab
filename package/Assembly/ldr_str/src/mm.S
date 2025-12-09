.global memzero
memzero:
    str xzr, [x0], #8
    subs x1, x1, #8
    b.gt memzero
    ret


.global memclr
memclr:
    mov x2, #0x5A5A
loop:
    str x2, [x0], #8
    subs x1, x1, #8
    b.gt loop
    ret
