CFLAGS+=-Wall -Wextra -pedantic
CFLAGS+=-Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wrestrict -Wnull-dereference -Wjump-misses-init -Wformat=2
CFLAGS+=-Wformat-overflow -Wformat-truncation -Wundef -fno-common -Wunused-parameter
libasm8051.a: avocet.o delegate.o readline.o strarray.o strcasecmp.o strcasecmp.o token.o
	cppcheck --quiet --suppress=unusedFunction --enable=all $(filter-out strcasecmp.c,$(subst .o,.c,$^))
	-splint -weak +quiet -nestedextern -predboolothers -boolops $(filter-out strcasecmp.c,$(subst .o,.c,$^))
	ar -rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<
