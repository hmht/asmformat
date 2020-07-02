CC=gcc
CFLAGS+=-Wall -Wextra -pedantic
#CFLAGS+=-Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wrestrict -Wnull-dereference -Wjump-misses-init -Wformat=2
#CFLAGS+=-Wformat-overflow -Wformat-truncation -Wundef -fno-common -Wunused-parameter
#CFLAGS+=-g
#CFLAGS+=-fsanitize=address -fsanitize=undefined -lasan
CFLAGS+=-DONLY_TABS

version:=1.0-1
debname:=asmformat_$(version)

.PHONY: all
all: asmformat $(debname).deb

asmformat asmformat.exe: program.c libasm8051/libasm8051.a
	#cppcheck --quiet --suppress=unusedFunction --enable=all $<
	$(CC) $(CFLAGS) $(filter-out $(MAKEFILE_LIST),$^) -o $@

$(debname).deb: $(debname)/usr/local/bin/asmformat $(debname)/DEBIAN/control
	dpkg-deb --build $(basename $@)
clean: | $(wildcard $(debname).deb)

$(debname)/%/asmformat: asmformat | $(debname)/%/
	install $< $@

$(debname)/%/:; mkdir -p $@

$(debname)/DEBIAN/control: debian.control | $(debname)/DEBIAN/
	install $< $@
	echo 'Version: $(version)' >> $@
clean: | $(wildcard $(debname)/)

%.mk: %.c
	$(CC) -MM $< > $@
.PHONY: clean
clean: | $(wildcard asmformat asmformat.exe)
	@test -z "$|" || echo rm -r $|
	@test -z "$|" || rm -r $|

git_modules:=$(shell git config --file .gitmodules --get-regexp path | cut -d' ' -f 2)
$(foreach m, $(git_modules), $m/$m.mk): .gitmodules
	git submodule update --init $(@D)
	touch --no-create $@

include libasm8051/libasm8051.mk

%.o: %.c
	$(CC) $(CFLAGS) -c $(filter-out $(MAKEFILE_LIST), $^) -o $@
