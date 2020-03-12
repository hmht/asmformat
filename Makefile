CC=gcc
CFLAGS+=-Wall -Wextra -pedantic
CFLAGS+=-Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wrestrict -Wnull-dereference -Wjump-misses-init -Wformat=2
CFLAGS+=-Wformat-overflow -Wformat-truncation -Wundef -fno-common -Wunused-parameter
#CFLAGS+=-g
#CFLAGS+=-fsanitize=address -fsanitize=undefined -lasan
CFLAGS+=-DONLY_TABS

asmformat: program.c libasm8051/token.o libasm8051/strarray.o libasm8051/avocet.o libasm8051/readline.o libasm8051/delegate.o libasm8051/strcasecmp.o
	cppcheck --quiet --suppress=unusedFunction --enable=all $(filter-out strcasecmp.c,$(subst .o,.c,$^))
	$(CC) $(CFLAGS) $^ -o $@

tests: asmformat
	sh test.sh

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	-rm -r *.o asmformat tests
