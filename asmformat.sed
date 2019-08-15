#!/bin/sed -f
# add colon after labels
s/^\(\w\+\)\s/\1:/
s/^\(\w\+\)$/\1:/

#remove colon for EQU, BIT, DATA, and XDATA
s/:\(\s*[eE][qQ][uU]\)/ \1/
s/:\(\s*[bB][iI][tT]\)/ \1/
s/:\(\s*[xX]\?[dD][aA][tT][aA]\)/ \1/
#also for xseg
s/^[xX][sS][eE][gG]:/xseg/

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

#add spaces after commas
s/,\([^ ]\)/, \1/g

#trim whitespace
s/\s\+$//g

# replace indentation (spaces->tabs)
# instructions
 # with arguments
s/^\s\+\(\b\w\+\b\)\s\+/\t\1\t/g
 #without arguments
s/^\s\+\(\b\w\+\b\)$/\t\1/g
# after labels
s/^\(\w\{1,6\}\b:\)\s*\(\b\w\+\b\)\s\+/\1\t\2\t/g
s/^\(\w\{7\}\b:\)\s*\(\b\w\+\b\)\s\+/\1\2\t/g
s/^\(\w\{8,\}\b:\)\s*\(\b\w\+\b\)\s\+/\1\n\t\2\t/g

# EQU, BIT, DATA, and XDATA
s/^\(\w\{1,7\}\b\)\s\+\([eE][qQ][uU]\|[bB][iI][tT]\|[xX]\?[dD][aA][tT][aA]\)/\1\t\t\t\2/g
s/^\(\w\{8,\}\b\)\s\+\([eE][qQ][uU]\|[bB][iI][tT]\|[xX]\?[dD][aA][tT][aA]\)/\1\t\t\2/g
s/^\(\w\{16,\}\b\)\s\+\([eE][qQ][uU]\|[bB][iI][tT]\|[xX]\?[dD][aA][tT][aA]\)/\1\t\2/g
# s/^\(\w\{24,\}\b\)\s\+\(equ\|bit\|x\?data\|\)/\1\t\2/g

# comment alignment
s/\(\t[^\t]\+\b\)\s\+;/\1\t;/g
s/^\(\t\b\w\+\b\s\+\);/\1\t;/g
s/\(\t[^\t]\{,7\}\b\s\+\);/\1\t;/g
s/\(\t[^\t]\{,15\}\b\s\+\);/\1\t;/g
s/\(\t[^\t]\{,23\}\b\s\+\);/\1\t;/g

# remove weird bytes
s/\x1A//g
s/\r//g
