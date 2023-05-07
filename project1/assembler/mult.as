        lw      0       1       mcand       store multiplicand in reg 1
        lw      0       2       mplier      store multiplier in reg 2
        lw      0       7       mcand       store multiplier in reg 7
        lw      0       3       one         use reg 3 as multiflier bit checker
        nor     2       2       2
check   nor     3       3       5
        nor     2       5       4           reg 4 = reg 2 & reg 3
        beq     3       4       radd        if reg 3 == reg 4
next    add     7       7       7
        add     3       3       3
        beq     0       3       end
        beq     0       0       check
radd    add     6       7       6           reg 6 is result, reg 6 += reg 7
        beq     0       0       next       
end     add     0       6       1
        halt
mcand   .fill   32766
mplier  .fill   12328
one     .fill   1
