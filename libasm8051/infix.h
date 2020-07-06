#include"trie/trie.h"
#include<stdint.h>

struct parseerror
{
	/** offset where the error happened */
	int at;
	/** statically-allocated string of what kind of data was expected */
	char const*expected;
	/** dynamically-allocated string, to be freed by the consumer, indicating what data was incorrect */
	char*got;
};

/**
 * solve an infix expression
 * @param e the expression that must be parsed and solved
 * @param sym defined variables/symbols
 * @param parseerror will be filled if there was an error
 */
extern uint16_t resolve_expression(char const*e, struct trie*sym, struct parseerror*err);
