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
extern bool callback_tokens(char const*filename, bool (*callback)(char**tokens, int linenr, void*data), void*data);
extern bool callback_tagged_token(char const*filename, bool (*callback)(struct tagged_token const*tt, int linenr, void*data), void*data);
