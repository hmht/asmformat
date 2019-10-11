#include"token.h"
#include<stdlib.h>
#include<string.h>
#include<stdio.h>

/******************************************************************************
 * Section: array datastructure
 *
 * dynamiccally sized array
 */

static void append(char***tokens, char*token)
{
	size_t ntokens = 0;
	while ( ( *tokens)[ntokens] )
	{
		ntokens += 1;
	}
	ntokens += 1;
	*tokens = reallocarray ( *tokens, ntokens + 1, sizeof (char** ));
	( *tokens)[ntokens - 1] = strdup (token);
	( *tokens)[ntokens] = 0;
}

extern void free_token_list(char**tokens)
{
	if ( !tokens)
	{
		return;
	}
	size_t i = 0;
	while (tokens[i] )
	{
		free (tokens[i] ); tokens[i] = 0;
		i += 1;
	}
	free (tokens); tokens = 0;
}

/******************************************************************************
 */

static bool is_whitespace(char c)
{
	return c == ' ' || c == '\t';
}

static size_t wordtokenizer(struct syntax const*syntax, char const**line, char**token)
{
	if ( syntax->is_comment ( *line) || syntax->is_macro ( *line))
	{
		strcpy ( *token, *line);
		*token = realloc ( *token, strlen ( *token) + 1);
		*line += strlen ( *line);
		return strlen ( *token);
	}

	if ( !syntax->is_word_char (true, **line))
	{
		if ( **line && ! is_whitespace ( **line))
		{
			fprintf (stderr, "unexpected character '%c' while looking for label, mnemonic, or declaration\n" , **line );
		}
		free ( *token); *token = 0;
		return 0;
	}

	char*w = *token;
	while (syntax->is_word_char (false, **line))
	{
		*w = **line;
		w += 1;
		*line += 1;
	}

	if ( **line == ':' )
	{
		*w = **line;
		w += 1;
		*line += 1;
	}

	*w = '\0';
	*token = realloc ( *token, strlen ( *token) + 1);
	return strlen ( *token);
}

static size_t argtokenizer(struct syntax const*syntax, char const**line, char**token)
{
	if (syntax->is_comment ( *line))
	{
		strcpy ( *token, *line);
		*token = realloc ( *token, strlen ( *token) + 1);
		*line += strlen ( *line);
		return strlen ( *token);
	}

	char*last_nonwhitespace = *token - 1;
	char*w = *token;
	bool is_in_string = false;
	while ( **line)
	{
		if ( !is_in_string
			&& ( **line == ',' || syntax->is_comment ( *line)))
		{
			break;
		}
		if ( !is_in_string && syntax->opens_string ( **line))
		{
			is_in_string = true;
		}
		else if (is_in_string && syntax->closes_string(*(*line - 1), **line))
		{
			is_in_string = false;
		}
		*w = **line;
		if ( !is_whitespace ( **line))
		{
			last_nonwhitespace = w;
		}
		w += 1;
		*line += 1;
	}

	w = last_nonwhitespace + 1;

	if ( **line == ',' )
	{
		*w = **line;
		w += 1;
		*line += 1;
	}

	*w = '\0';
	*token = realloc ( *token, strlen ( *token) + 1);
	return strlen ( *token);
}

static size_t blobtokenizer(struct syntax const*syntax, char const**line, char**token)
{
	(void) syntax;
	char*last_nonwhitespace = *token - 1;
	char*w = *token;
	while ( **line)
	{
		*w = **line;
		if ( !is_whitespace ( **line))
		{
			last_nonwhitespace = w;
		}
		w += 1;
		*line += 1;
	}
	w = last_nonwhitespace + 1;
	*w = '\0';
	*token = realloc ( *token, strlen ( *token) + 1);
	return strlen ( *token);
}

extern bool split_line_into_parts(struct syntax const*syntax, char const*line, char***tokens)
{
	if ( ! line)
	{
		return false;
	}
	size_t ( *tokenizer)(struct syntax const*, char const**, char** ) = &wordtokenizer;
	while ( *line)
	{
		while ( is_whitespace ( *line))
		{
			line += 1;
		}
		char*token = malloc (strlen (line) + 1);
		if ( !tokenizer (syntax, &line, &token))
		{
			fprintf (stderr, "empty token from %stokenizer?\n", tokenizer == wordtokenizer ? "word" : tokenizer == argtokenizer ? "arg" : "blob" );
			free (token); token = 0;
			return ! *line;
		}
		if (syntax->has_arguments (token))
		{
			tokenizer = &argtokenizer;
		}
		else if (syntax->has_blob (token))
		{
			tokenizer = &blobtokenizer;
		}
		append (tokens, token);
		free (token); token = 0;
	}
	return true;
}
