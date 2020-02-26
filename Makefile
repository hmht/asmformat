CC=gcc
CFLAGS+=-g
CFLAGS+=-Wall -Wextra -pedantic
#CFLAGS+=-fsanitize=address -fsanitize=undefined -lasan
CFLAGS+=-DONLY_TABS

asmformat: program.c libasm8051/token.o libasm8051/strarray.o libasm8051/avocet.o libasm8051/readline.o
	$(CC) $(CFLAGS) $^ -o $@

tests: asmformat
	sh test.sh

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	-rm -r *.o asmformat tests
