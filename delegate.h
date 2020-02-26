#include<stdbool.h>
enum token_t {
	blob,
	label,
	mnemonic_or_declaration,
	argument,
	comment,
	newline,
};

struct tagged_token {
	enum token_t type;
	char*token;
};
#include<stdio.h>
extern bool callback_tokens(FILE*file, bool (*callback)(char**tokens, int linenr, void*data), void*data);
extern bool callback_tagged_token(FILE*file, bool (*callback)(struct tagged_token const*tt, int linenr, void*data), void*data);
