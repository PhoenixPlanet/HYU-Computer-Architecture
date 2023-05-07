        lw      0       2       target
        lw      0       1       one
loop    beq     2       3       end
        add     4       3       4
        add     3       1       3
        beq     0       0       loop
end     noop
        add     0       3       1
        halt
target  .fill   10
one     .fill   1
