path:=$(dir $(lastword $(MAKEFILE_LIST)))

$(path)strcasecmp.o: CC=gcc
$(path)strcasecmp.o: CFLAGS=-O3
$(path)delegate.o: $(addprefix $(path), token.o readline.o strarray.o avocet.o)
$(path)token.o: $(path)avocet.o $(path)strarray.o
$(path)infix.o: $(path)trie/trie.o

objs:=$(addprefix $(path), avocet.o delegate.o readline.o strarray.o token.o infix.o trie/trie.o)

# windows doesn't have strcasecmp
objs+=$(path)strcasecmp.o

$(path)libasm8051.a: $(objs)
	$(AR) -rcs $@ $^

clean: | $(wildcard $(path)libasm8051.a $(objs) $(path)strcasecmp.o)

undefine objs
include $(path)trie/trie.mk
undefine path
