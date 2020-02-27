#include"delegate.h"
#include<stdlib.h>
#include<stdio.h>
extern bool callback_tokens(FILE*file, bool (*callback)(char**tokens, int linenr, void*data), void*data)
{
	int linenr = 1;
	bool more_lines;
	do
	{
		#include"readline.h"
		#include"token.h"
		#include"strarray.h"
		char*input = 0;
		more_lines = readline (file, &input);
		if ( !more_lines && !input[0] )
		{
			free (input); input = 0;
			break;
		}
		char**tokens = malloc (sizeof (char* ));
		*tokens = 0;
		if ( !split_line_into_parts (input, &tokens))
		{
			fprintf (stderr, "token splitting failed on line %d:\n%s\n" , linenr, input);
			free (input); input = 0;
			strarray_free (tokens); tokens = 0;
			fclose (file); file = 0;
			return false;
		}
		if (!callback(tokens, linenr, data))
		{
			free (input); input = 0;
			strarray_free (tokens); tokens = 0;
			fclose (file); file = 0;
			return false;
		}
		linenr += 1;
		free (input); input = 0;
		strarray_free (tokens); tokens = 0;
	} while (more_lines);
	fclose (file); file = 0;
	return true;
}

struct lex_callback {
	bool (*callback)(struct addressed const*a, void*data);
	struct addressed*last;
	void*data;
};

static bool dispatch_addressed(bool (*callback)(struct addressed const*a, void*data), struct addressed*a, void*data)
{
	#include"strarray.h"
	callback (a, data);
	strarray_free (a->label); a->label = 0;
	strarray_free (a->argument); a->argument = 0;
	strarray_free (a->comment); a->comment = 0;
	free (a->mnemonic_or_declaration); a->mnemonic_or_declaration = 0;
}

#include<string.h>
static bool lex(char**token, int linenr, void*vc)
{
	struct lex_callback const*c = vc;
	enum state
	{
		seen_start,
		seen_label,
		seen_mnemonic_or_declaration,
		seen_argument,
		seen_comment,
		seen_newline,
	} state = seen_start;
	int i = 0;
	while (token[i])
	{
		#include"syntax.h"
		#include"strarray.h"
		char*current = token[i];
		i += 1;
		if (state < seen_newline && is_newline (current))
		{
			state = seen_newline;
			continue;
		}
		if (state <= seen_comment && is_comment (current))
		{
			c->last->line_to = linenr;
			strarray_append(&c->last->comment, current);
			state = seen_comment;
			continue;
		}
		if (state <= seen_argument
			&& seen_mnemonic_or_declaration <= state
			&& !is_newline (current)
			&& !is_comment (current)
			&& !is_mnemonic_or_declaration (current))
		{
			c->last->line_to = linenr;
			strarray_append(&c->last->argument, current);
			state = seen_argument;
			continue;
		}
		if (state < seen_mnemonic_or_declaration
			&& is_mnemonic_or_declaration (current))
		{
			if (c->last->mnemonic_or_declaration) {
				dispatch_addressed(c->callback, c->last, c->data);
				c->last->line_from = linenr;
			}
			c->last->line_to = linenr;
			c->last->mnemonic_or_declaration = strdup(current);
			state = seen_mnemonic_or_declaration;
			continue;
		}
		if (state < seen_label
			&& has_blob (current))
		{
			if (c->last->mnemonic_or_declaration) {
				dispatch_addressed(c->callback, c->last, c->data);
				c->last->line_from = linenr;
			}
			c->last->line_to = linenr;
			c->last->mnemonic_or_declaration = strdup(current);
			state = seen_mnemonic_or_declaration;
			continue;
		}
		if (state < seen_label
			&& !is_newline (current)
			&& !is_comment (current)
			&& !is_mnemonic_or_declaration (current))
		{
			if (c->last->mnemonic_or_declaration) {
				dispatch_addressed(c->callback, c->last, c->data);
				c->last->line_from = linenr;
			}
			c->last->line_to = linenr;
			strarray_append (&c->last->label, current);
			state = seen_label;
			continue;
		}

		fprintf (stderr, "unexpected token \"%s\" on line %d;" , current, linenr);
		switch (state)
		{
		break; case seen_start:
			fprintf (stderr, "expected label, mnemonic, declaration, or comment as first token.\n");
		break; case seen_label:
			fprintf (stderr, "expected mnemonic, declaration, or comment after a label (\"%s\").\n", current[-1]);
		break; case seen_mnemonic_or_declaration:
			fprintf (stderr, "expected argument or comment after mnemonic/declaration \"%s\"\n", current[-1]);
		case seen_argument:
			fprintf (stderr, "expected argument or comment after argument \"%s\"\n", current[-1]);
		break; case seen_comment:
			fprintf (stderr, "stray token after comment \"%s\"\n", current[-1]);
		break; case seen_newline:
		default:
			fprintf (stderr, "token consumer error (seen=%d)\n" , state);
		}
		return false;
	}
	return true;
}

extern bool callback_tagged_token(FILE*file, bool (*callback)(struct addressed const*a, void*data), void*data)
{
	struct addressed a = {};
	struct lex_callback c = { callback, &a, data };
	bool const ret = callback_tokens(file, lex, &c);
	if (ret && c.last->mnemonic_or_declaration) {
		return dispatch_addressed (callback, c.last, data);
	}
	return ret;
}

