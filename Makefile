CC=gcc
CFLAGS+=-Wall -Wextra -pedantic
CFLAGS+=-Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wrestrict -Wnull-dereference -Wjump-misses-init -Wformat=2
CFLAGS+=-Wformat-overflow -Wformat-truncation -Wundef -fno-common -Wunused-parameter
#CFLAGS+=-g
#CFLAGS+=-fsanitize=address -fsanitize=undefined -lasan
CFLAGS+=-DONLY_TABS

asmformat.exe asmformat: program.c libasm8051/libasm8051.a
	#cppcheck --quiet --suppress=unusedFunction --enable=all $<
	$(CC) $(CFLAGS) $(filter-out $(MAKEFILE_LIST),$^) -o $@

.PHONY: clean
clean: | $(wildcard asmformat)
	@test -z "$|" || echo rm $|
	@test -z "$|" || rm $|

$(foreach m, $(shell git submodule status | cut -d' ' -f2), $m/$m.mk): .gitmodules
	git submodule update --init $(@D)
	touch --no-create $@

include libasm8051/libasm8051.mk

%.o: %.c
	$(CC) $(CFLAGS) -c $(filter-out $(MAKEFILE_LIST), $^) -o $@
