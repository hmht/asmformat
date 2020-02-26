#include<stdbool.h>
#include<stdio.h>
struct addressed {
	int line_from;
	int line_to;
	char**label;
	char*mnemonic_or_declaration;
	char**argument;
	char**comment;
};
extern bool callback_tokens(FILE*file, bool (*callback)(char**tokens, int linenr, void*data), void*data);
extern bool callback_tagged_token(FILE*file, bool (*callback)(struct addressed const*a, void*data), void*data);
