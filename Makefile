CC:=clang
CFLAGS+=-Wall -Wextra -pedantic

ifeq ($(CC),gcc)
CFLAGS+=-Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wrestrict -Wnull-dereference -Wjump-misses-init -Wformat=2
CFLAGS+=-Wformat-overflow -Wformat-truncation -Wundef -fno-common -Wunused-parameter
endif

CFLAGS+=-fsanitize=address
LDFLAGS+=-lasan

all: libasm8051.a
#	cppcheck --quiet --suppress=unusedFunction --enable=all $(filter-out strcasecmp.c,$(subst .o,.c,$^))
#	-splint -posixlib -weak +quiet -nestedextern -predboolothers -boolops $(filter-out strcasecmp.c,$(subst .o,.c,$^))

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean: | $(wildcard libasm8051.a)
	@test -z "$|" || echo rm $|
	@test -z "$|" || rm $|

include libasm8051.mk
