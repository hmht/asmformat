#!/bin/sed -f

#remove some compiler directives
/^\$pw=.*/d

#translate hexadecimal literals from 0x0f to 0fh
s/\$\([0123456789abcdefABCDEF]\+\)\b/0\1h/g
