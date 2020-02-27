#include"token.h"
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
static bool is_whitespace(char c)
{
	return c == ' ' || c == '\t';
}

static size_t wordtokenizer(char const**line, char**token)
{
	#include"syntax.h"
	if ( is_comment ( *line) || has_blob ( *line))
	{
		strcpy ( *token, *line);
		*token = realloc ( *token, strlen ( *token) + 1);
		*line += strlen ( *line);
		return strlen ( *token);
	}

	if ( !is_word_char (true, **line))
	{
		if ( **line && ! is_whitespace ( **line))
		{
			fprintf (stderr, "unexpected character '%c' while looking for label, mnemonic, or declaration\n" , **line );
		}
		free ( *token); *token = 0;
		return 0;
	}

	char*w = *token;
	while (is_word_char (false, **line))
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

static size_t argtokenizer(char const**line, char**token)
{
	#include"syntax.h"
	if (is_comment ( *line))
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
			&& ( **line == ',' || is_comment ( *line)))
		{
			break;
		}
		if ( !is_in_string && opens_string ( **line))
		{
			is_in_string = true;
		}
		else if (is_in_string && closes_string(*(*line - 1), **line))
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

	*w = '\0';
	*token = realloc ( *token, strlen ( *token) + 1);
	return strlen ( *token);
}

static size_t blobtokenizer(char const**line, char**token)
{
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

extern bool split_line_into_parts(char const*line, char***tokens)
{
	#include"syntax.h"
	if ( ! line)
	{
		return false;
	}
	size_t ( *tokenizer)(char const**, char** ) = &wordtokenizer;
	while ( *line)
	{
		#include"strarray.h"
		while ( is_whitespace ( *line))
		{
			line += 1;
		}
		char*token = malloc (strlen (line) + 1);
		if ( !tokenizer (&line, &token))
		{
			fprintf (stderr, "empty token from %stokenizer?\n", tokenizer == wordtokenizer ? "word" : tokenizer == argtokenizer ? "arg" : "blob" );
			free (token); token = 0;
			return ! *line;
		}
		if (tokenizer == argtokenizer)
		{
			line += *line == ',' ;
		}
		else if (has_arguments (token))
		{
			tokenizer = &argtokenizer;
		}
		else if (has_blob (token))
		{
			tokenizer = &blobtokenizer;
		}
		strarray_append (tokens, token);
		free (token); token = 0;
	}
	return true;
}
