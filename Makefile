libasm8051.a: avocet.o delegate.o readline.o strarray.o strcasecmp.o strcasecmp.o token.o
	ar -rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<
