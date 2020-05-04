#include"delegate.h"
#include<stdlib.h>
#include<stdio.h>
static bool
read_tokens(FILE*file, char***tokens)
{
	#include"readline.h"
	#include"token.h"
	#include"strarray.h"
	char*input = 0;
	bool const more_lines = readline (file, &input);
	if ( !more_lines && input && !input[0] )
	{
		free (input); input = 0;
		return false;
	}
	if ( !split_line_into_parts (input, tokens))
	{
		fprintf (stderr, "token splitting failed on:\n%s\n" , input);
		free (input); input = 0;
		return false;
	}
	free (input); input = 0;
	return more_lines;
}

extern bool callback_tokens(FILE*file, bool (*callback)(char**tokens, int linenr, void*data), void*data)
{
	int linenr = 1;
	bool more_lines = true;
	while (more_lines)
	{
		#include"strarray.h"
		char**tokens = malloc (sizeof (char* ));
		if (!tokens)
		{
			return false;
		}
		*tokens = 0;
		more_lines = read_tokens (file, &tokens);
		if (!callback(tokens, linenr, data))
		{
			strarray_free (tokens); tokens = 0;
			return false;
		}
		linenr += 1;
		strarray_free (tokens); tokens = 0;
	}
	return true;
}

#include<string.h>
static bool
lex(char**token, int linenr, struct addressed*out)
{
	enum state
	{
		seen_start,
		seen_label,
		seen_mnemonic_or_declaration,
		seen_argument,
		seen_comment,
		seen_newline,
	} state = seen_start;
	out->line_from = linenr;
	out->line_to = linenr;
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
			strarray_append(&out->comment, current);
			state = seen_comment;
			continue;
		}
		if (state <= seen_argument
			&& seen_mnemonic_or_declaration <= state
			&& !is_newline (current)
			&& !is_comment (current)
			&& !is_mnemonic_or_declaration (current))
		{
			strarray_append(&out->argument, current);
			state = seen_argument;
			continue;
		}
		if (state < seen_mnemonic_or_declaration
			&& is_mnemonic_or_declaration (current))
		{
			out->mnemonic_or_declaration = strdup(current);
			state = seen_mnemonic_or_declaration;
			continue;
		}
		if (state < seen_label
			&& has_blob (current))
		{
			out->mnemonic_or_declaration = strdup(current);
			state = seen_mnemonic_or_declaration;
			continue;
		}
		if (state < seen_label
			&& !is_newline (current)
			&& !is_comment (current)
			&& !is_mnemonic_or_declaration (current))
		{
			strarray_append (&out->label, current);
			state = seen_label;
			continue;
		}

		fprintf (stderr, "unexpected token \"%s\" on line %d;" , current, linenr);
		switch (state)
		{
		break; case seen_start:
			fprintf (stderr, "expected label, mnemonic, declaration, or comment as first token.\n");
		break; case seen_label:
			fprintf (stderr, "expected mnemonic, declaration, or comment after a label (\"%s\").\n", token[i-1]);
		break; case seen_mnemonic_or_declaration:
			fprintf (stderr, "expected argument or comment after mnemonic/declaration \"%s\"\n", token[i-1]);
		break; case seen_argument:
			fprintf (stderr, "expected argument or comment after argument \"%s\"\n", token[i-1]);
		break; case seen_comment:
			fprintf (stderr, "stray token after comment \"%s\"\n", token[i-1]);
		break; case seen_newline:
		default:
			fprintf (stderr, "token consumer error (seen=%d)\n" , state);
		}
		return false;
	}
	return true;
}

static bool
read_lex(FILE*file, int linenr, struct addressed*a)
{
	#include"strarray.h"
	char**token = {0};
	bool more_lines = read_tokens(file, &token);
	if (token)
	{
		more_lines &= lex (token, linenr, a);
		strarray_free (token); token = 0;
	}
	return more_lines;
}

extern void
free_addressed(struct addressed*a)
{
	#include"strarray.h"
	strarray_free (a->label); a->label = 0;
	strarray_free (a->argument); a->argument = 0;
	strarray_free (a->comment); a->comment = 0;
	free (a->mnemonic_or_declaration); a->mnemonic_or_declaration = 0;
}

extern bool
callback_lexed_line(FILE*file, bool (*callback)(struct addressed const*, void*), void*data)
{
	bool rv = true;
	bool more_lines = true;
	int linenr = 1;
	while (rv && more_lines)
	{
		struct addressed a = {0};
		more_lines = read_lex (file, linenr, &a);
		if (!more_lines
			&& !a.label
			&& !a.mnemonic_or_declaration
			&& !a.argument
			&& !a.comment)
		{
			break;
		}
		rv &= callback (&a, data);
		free_addressed (&a);
		linenr += 1;
	}
	return rv;
}

static void
merge_addressed(struct addressed*dest, struct addressed*src)
{
	#include"strarray.h"
	strarray_merge (&dest->label, src->label);

	//assert (! (dest->mnemonic_or_declaration && src->mnemonic_or_declaration));
	if (!dest->mnemonic_or_declaration)
	{
		 dest->mnemonic_or_declaration = src->mnemonic_or_declaration;
	}

	strarray_merge (&dest->argument, src->argument);
	strarray_merge (&dest->comment, src->comment);
}

extern bool
callback_addressed(FILE*file, bool (*callback)(struct addressed const*, void*), void*data)
{
	bool rv = true;
	bool more_lines = true;
	int linenr = 1;
	struct addressed a = {0};
	while (rv && more_lines)
	{
		struct addressed l = {0};
		more_lines = read_lex (file, linenr, &l);
		if (a.mnemonic_or_declaration
			&& (l.mnemonic_or_declaration || l.label))
		{
			rv &= callback (&a, data);
			free_addressed (&a);
			memcpy(&a, &l, sizeof a);
		} else {
			merge_addressed (&a, &l);
		}
		a.line_to = linenr;
		linenr += 1;
	}
	if (rv && a.mnemonic_or_declaration)
	{
		rv &= callback (&a, data);
	}
	free_addressed (&a);
	return rv;
}
