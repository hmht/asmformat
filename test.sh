set -e
readonly gitserver="http://192.168.0.235:3000"

function convallasm() {
find . -name '*.ASM' -exec ../../asmformat \{\} \;
}

function testproj() {
	test -d $1 && rm -r $1
	git clone $gitserver/prod/$1.git --depth 1
	cd $1
	convallasm
	# first test; does it output compilable code?
	make
	cp -r . ../pass-1-$1
	convallasm
	cd ..
	# second test: is it idempotent?
	diff -u {pass-1-,}$1
}

gcc -g -o asmformat program.c -Wall -Wextra -pedantic -fsanitize=address -fsanitize=undefined -lasan -DONLY_TABS
rm -rf test

test -d test || mkdir test
cd test
testproj na
testproj dtb
testproj deb

echo
echo 'ALL TESTS PASSED'
