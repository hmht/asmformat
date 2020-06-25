module_path:=$(dir $(lastword $(MAKEFILE_LIST)))

objs:=$(addprefix $(module_path), avocet.o delegate.o readline.o strarray.o token.o infix.o trie/trie.o)

$(objs): $(lastword $(MAKEFILE_LIST))

# windows doesn't have strcasecmp
objs+=$(module_path)strcasecmp.o
$(module_path)strcasecmp.o: CFLAGS=-O3

$(module_path)libasm8051.a: $(objs)
	$(AR) -rcs $@ $^

.PHONY: clean
clean: | $(wildcard $(module_path)libasm8051.a $(objs) $(module_path)strcasecmp.o)

git_modules:=$(shell git -C ./$(module_path) config --file .gitmodules --get-regexp path | cut -d' ' -f 2)
$(foreach m, $(git_modules), $(module_path)$m/$m.mk): $(module_path).gitmodules
	git -C $(dir $<) submodule update --init $(notdir $(@D))
	touch --no-create $@

include $(module_path)trie/trie.mk
include $(patsubst %.o, %.mk, $(objs))
undefine objs
undefine module_path
