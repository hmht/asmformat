CC=gcc
CFLAGS+=-g
CFLAGS+=-Wall -Wextra -pedantic
CFLAGS+=-fsanitize=address -fsanitize=undefined -lasan
CFLAGS+=-DONLY_TABS

tests: asmformat
	sh test.sh

asmformat: program.c token.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $^

clean:
	-rm -r *.o asmformat tests
