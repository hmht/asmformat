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

# translate includes
s/[iI][nN][cC][lL][uU][dD][eE]\s\+\(\w\+\)\.[aA][sS][mM]/include \L\1.\UASM/

# translate some compiler directives
s/^\$pw\s*=\?\([0-9]\+\).*$/$pw 255/

#translate bit literals from %0101 to 0101b
s/%\([01]\+\)\b/\1b/g
