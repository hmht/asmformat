set -e
readonly gitserver="http://forge.dev.hollandmechanics.com."

function convallasm() {
find . -name '*.ASM' -exec ../../asmformat \{\} \;
}

function testproj() {
	test -d $1 && rm -r $1
	git clone $gitserver/prod/$1.git --depth 1
	cd $1
	make
	mv $1bim.bin orig.bin
	convallasm
	# first test; does it output compilable code?
	make
	# second test: does it produce the same binary?
	~/bin/radiff2 -szx $1bim.bin orig.bin
	cp -r . ../pass-1-$1
	convallasm
	cd ..
	# third test: is it idempotent?
	diff -u {pass-1-,}$1
}

rm -rf test

test -d test || mkdir test
cd test
testproj na
testproj dtb
testproj deb

echo
echo 'ALL TESTS PASSED'
