#!/bin/sed -f

# add parentheses around some negative numbers
s/#[lL][oO][wW]\s\+\(-[0-9]\+\)/#low (\1)/
s/#[hH][iI][gG][hH]\s\+\(-[0-9]\+\)/#high (\1)/

#delete some compiler directives
/^\$allpublic/d
/^\$bs/d
/^\$\(copyright\|cr\)/d
/^\$showmacs/d
/^\$subtitle/d
/^\$version/d
/^\$pl=.*/d
/^\$pg/d
/^\$ingnorecase/d

#translate hexadecimal literals from 0x0f to 0fh
s/\$\([0123456789abcdefABCDEF]\+\)\b/0\1h/g
