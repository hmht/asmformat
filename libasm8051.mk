path:=$(dir $(lastword $(MAKEFILE_LIST)))

objs:=$(addprefix $(path), avocet.o delegate.o readline.o strarray.o token.o infix.o trie/trie.o)

$(objs): $(lastword $(MAKEFILE_LIST))

# windows doesn't have strcasecmp
objs+=$(path)strcasecmp.o
$(path)strcasecmp.o: CFLAGS=-O3

$(path)libasm8051.a: $(objs)
	$(AR) -rcs $@ $^

.PHONY: clean
clean: | $(wildcard $(path)libasm8051.a $(objs) $(path)strcasecmp.o)

$(foreach m, $(shell git submodule status | cut -d' ' -f2), $(path)/$m/$m.mk): $(path)/.gitmodules
	git submodule update --init $(@D)
	touch --no-create $@

undefine objs
include $(path)trie/trie.mk
undefine path
