        lw      0       1       a
        lw      0       2       b
        lw      0       6       sumpro
        jalr    6       7
        lw      0       1       result
        halt
        noop
        noop
sum     add     1       2       3
        sw      0       3       result
        jalr    7       6
a       .fill   15
b       .fill   17
result  .fill   0
sumpro  .fill   8
