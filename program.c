/*
 *                              Architecture
 *
 * Documentation last updated on 2019-09-27
 * 1. File is opened or stdin is used
 * 2. One line is read into dynamically allocated memory.
 * 3. The line is cut into "tokens". There are three tokenizers:
 *      - one for "wordish tokens" which are space-delimited
 *      - one for "argumentish tokens"; comma-delimited, and can have strings
 *      - one for comments, which start at a ; and end at the newline
 *    Note that this requires the tokenizer to already sort of understand the
 *    syntax, so it's sort of like it is a half-assed classifier. Not sure if
 *    this is a good idea.
 * 4. The tokens are gathered into an array (dynamically resized)
 * 5. The whole array is analized (tokens are classified by contents and
 *    relative position), and each is written out with prettier formatting.
 *    It currently assumes that each line can be independently formatted. I.e.
 *    there is no contextual formatting. If you want to go the full mile,
 *    contextual formatting gives the best results, you can prettify things
 *    like tables and make elastic tab stops.
 * 6. If there are no errors, the output file will overwrite the input file
 *
 *
 * File contents:
 * Section: classifiers
 * Section: formatter helpers
 * Section: token formatters
 * Section: formatting dispatcher
 * Section: tokenizers
 * Section: input handlers
 * Section: array datastructure
 * Section: main
 *
 */

#include<stdbool.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<ctype.h>

/******************************************************************************
 * Section: configuration
 */

#ifndef INDENT_WIDTH
#	define INDENT_WIDTH 8
#endif

#ifndef MNEMONIC_COLUMN
#	define MNEMONIC_COLUMN INDENT_WIDTH
#endif

#ifndef DECLARATOR_COLUMN
#	define DECLARATOR_COLUMN (2 * INDENT_WIDTH)
#endif

#ifndef ARGUMENT_COLUMN
#	define ARGUMENT_COLUMN (MNEMONIC_COLUMN + INDENT_WIDTH)
#endif

#ifndef COMMENT_COLUMN
#	define COMMENT_COLUMN (ARGUMENT_COLUMN + 3 * INDENT_WIDTH)
#endif

// TODO: tabs for some alignment, spaces for other alignment
#if defined(ONLY_TABS)
#	ifndef TAB_WIDTH
#		define TAB_WIDTH 8
#	endif
#	if TAB_WIDTH != INDENT_WIDTH
#		error "cannot have different indent width and tab width"
#	endif
#	ifdef ONLY_TABS
#		if (COMMENT_COLUMN != 4 * TAB_WIDTH) && (COMMENT_COLUMN != 5 * TAB_WIDTH) && (COMMENT_COLUMN != 6 * TAB_WIDTH)
#			error "comment column must be multiple of tab width"
#		endif
#	endif
#endif

/******************************************************************************
 * Section: classifiers
 *
 * figure out what kind of token you have
 */

static bool is_allcaps(char const*token)
{
	if ( !token)
	{
		return false;
	}

	while ( *token)
	{
		if ( 'a' <= *token && *token <= 'z' )
		{
			return false;
		}
		token += 1;
	}
	return true;
}

static bool is_newline(char const*token)
{
	return !token;
}

static bool is_comment(char const*token)
{
	if ( !token)
	{
		return false;
	}
	return token[0] == ';' ;
}

static bool is_macro(char const*token)
{
	if ( !token)
	{
		return false;
	}
	return token[0] == '$' ;
}

static bool is_segment_label(char const*token)
{
	if ( !token)
	{
		return false;
	}
	static char const*const addressable_declarator[] = {
		"xseg",
		"cseg",
		0
	};
	for (char const*const*p = addressable_declarator ; *p ; p += 1)
	{
		if (!strcasecmp ( *p, token))
		{
			return true;
		}
	}
	return false;
}

static bool is_addressable_declaration(char const*token)
{
	if ( !token)
	{
		return false;
	}
	static char const*const addressable_declarator[] = {
		"ds",
		"db",
		"dw",
		0
	};
	for (char const*const*p = addressable_declarator ; *p ; p += 1)
	{
		if (!strcasecmp ( *p, token))
		{
			return true;
		}
	}
	return false;
}

static bool is_declaration(char const*token)
{
	if ( !token)
	{
		return false;
	}
	if (is_addressable_declaration (token))
	{
		return true;
	}
	static char const*const declarators[] = {
		"include",
		"bit",
		"data",
		"xdata",
		"equ",
		"code",
		"org",
		"end",
		0
	};
	for (char const*const*p = declarators ; *p ; p += 1)
	{
		if ( !strcasecmp ( *p, token))
		{
			return true;
		}
	}
	return false;
}

static bool is_mnemonic(char const*token)
{
	if ( !token)
	{
		return false;
	}
	static char const*const mnemonics[] = {
		"acall", "addc", "add", "ajmp", "anl", "cjne", "call", "clr",
		"cpl", "da", "dec", "div", "djnz", "inc", "jbc", "jb", "jc",
		"jmp", "jnb", "jnc", "jz", "jnz", "lcall", "ljmp", "mov",
		"movc", "movx", "mul", "nop", "orl", "pop", "push", "reti",
		"ret", "xchd", "xch", "xrl", "rlc", "rl", "rrc", "rr", "setb",
		"sjmp", "subb", "swap",
		0
	};
	for (char const*const*p = mnemonics ; *p ; p += 1)
	{
		if ( !strcasecmp ( *p, token))
		{
			return true;
		}
	}
	return false;
}

static bool is_mnemonic_or_declaration(char const*token)
{
	if ( !token)
	{
		return false;
	}
	return is_declaration (token) || is_mnemonic (token);
}

static bool isWordChar(bool first, char c)
{
	return ( 'A' <= c && c <= 'Z' )
		|| ( 'a' <= c && c <= 'z' )
		|| c == '_' || c == '?'
		|| (first? false : ( '0' <= c && c <= '9' ));
}

/******************************************************************************
 * Section: formatter helpers
 */

static int pad_to_column(int from, int to, FILE*ofp)
{
	if (from < to)
	{
		for (int needed_spaces = (to - from)
			#if defined(ONLY_TABS)
			/ INDENT_WIDTH + ( !! ((to - from) % INDENT_WIDTH))
			#endif
			; needed_spaces
			; needed_spaces -= 1)
		{
			fprintf (ofp,
			#if defined(ONLY_TABS)
				"\t" );
			#else
				" " );
			#endif
		}
	}
	return
		#if defined(ONLY_TABS)
		((to / TAB_WIDTH) * TAB_WIDTH) - from
		#else
		to - from
		#endif
		;
}

static void lowercase_string(char*lower, char const*upper)
{
	while ( *upper)
	{
		*lower = tolower ( *upper);
		upper += 1;
		lower += 1;
	}
	*lower = '\0';
}

static void normalize_label(char*dest, char const*label)
{
	if ( !label)
	{
		return;
	}
	if (is_allcaps (label))
	{
		lowercase_string (dest, label);
	} else {
		*dest = tolower ( *dest);
	}

	return;
}

static void normalize_words_in(char*string_out)
{
	if ( !string_out)
	{
		return;
	}
	char word[strlen (string_out) + 1];
	char*wp = word;
	char*string_in = string_out;
	bool is_in_string = false;
	while (*string_in)
	{
		if ( *string_in == '\'' )
		{
			is_in_string = !is_in_string;
		}
		if ( !is_in_string && isWordChar (wp == word, *string_in))
		{
			*wp = *string_in;
			wp += 1;
			string_in += 1;
		} else {
			if (wp != word)
			{
				*wp = '\0';
				char tmp = *string_in;
				normalize_label (string_out, word);
				string_out += strlen (word);
				*string_out = tmp;
				wp = word;
			}
			string_in += 1;
			string_out += 1;
		}
	}
	if (wp != word)
	{
		*wp = '\0';
		if (strcmp ( "ASM" , word))
		{
			normalize_label (string_out, word);
		}
		string_out += strlen (word);
		wp = word;
	}
}

static bool fix_binary_literal_at(char*literal, int width)
{
	if ( width < 8)
	{
		fprintf (stderr, "binary number too short\n" );
		return false;
	} else {
	// TODO maybe check for junk at end of %bin
		while (literal[1] == '0'
			|| literal[1] == '1' )
		{
			*literal = literal[1];
			literal += 1;
		}
		*literal = 'b';
	}
	return true;
}

static bool fix_binary_notations(char*label)
{
	if ( !label)
	{
		return false;
	}
	bool is_in_string = false;
	int looking_for_binary = 8;
	char*writeout = 0;
	while ( *label)
	{
		if ( *label == '\'' )
		{
			is_in_string = !is_in_string;
		} else switch ( *label)
		{
		default:
			if (writeout)
			{
				if (fix_binary_literal_at (writeout, 8 - looking_for_binary))
				{
					writeout = 0;
				} else {
					return false;
				}
			}
		break; case '%':
			if (writeout)
			{
				fprintf (stderr, "nested binary number\n" );
				return false;
			}
			writeout = label;
			looking_for_binary = 8;
		break;
		case '0':
		case '1':
			if (writeout)
			{
				if ( !looking_for_binary)
				{
					fprintf (stderr, "binary number too long\n" );
					return false;
				}
				looking_for_binary -= 1;
			}
		}
		label += 1;
	}
	if (writeout)
	{
		return fix_binary_literal_at (writeout, 8 - looking_for_binary);
	}
	return true;
}

/******************************************************************************
 * Section: token formatters
 *
 * write out tokens with the desired formatting
 */

static void format_comment(char const*token, int column, FILE*ofp)
{
	if (column != 0)
	{
		(void) pad_to_column (column, COMMENT_COLUMN, ofp);
	}
	fprintf (ofp, "%s" , token);
}

static int format_mnemonic_or_declaration (char const*token, int const column, FILE*ofp)
{
	int min_column = is_declaration (token) ? DECLARATOR_COLUMN : MNEMONIC_COLUMN;
	if (min_column <= column && is_declaration (token) && !is_addressable_declaration (token))
	{
		min_column = column + 1;
	}
	int const chars_printed = pad_to_column (column, min_column, ofp);

	char lowercase_token[strlen (token) + 1];
	lowercase_string(lowercase_token, token);

	return chars_printed + fprintf (ofp, "%s%s" , lowercase_token, is_declaration (token) ? " " : "" );
}

static int format_argument (char const*token, FILE*ofp)
{
	bool const needs_space = ',' == token[strlen (token) - 1];
	char*maybe_space = needs_space? " " : "" ;

	// TODO: parens around negative numbers

	char*ntoken = strdup (token);
	normalize_words_in (ntoken);

	int printed = fprintf (ofp, "%s%s" , ntoken, maybe_space);
	free (ntoken);
	return printed;
}

static int format_label (char const*token, bool may_need_newline, bool colon_allowed, FILE*ofp)
{
	bool const has_colon = ':' == token[strlen (token) - 1];
	bool const needs_colon = colon_allowed
		&& !has_colon
		&& !is_segment_label (token);
	bool const needs_newline = may_need_newline
		&& MNEMONIC_COLUMN < (strlen (token) - has_colon);

	char*maybe_colon = needs_colon? ":" : "" ;
	char*maybe_newline = needs_newline? "\n" : "" ;

	char*ntoken = strdup (token);
	normalize_label (ntoken, token);
	size_t column = ( !needs_newline) * fprintf (ofp, "%s%s%s"
		, ntoken
		, maybe_colon
		, maybe_newline);
	free (ntoken); ntoken = 0;

	return column;
}

/******************************************************************************
 * Section: formatting dispatcher
 *
 * delegate control to token formatter
 */

static bool format(char**token, int linenr, FILE*ofp)
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
	int column = 0;
	int i = 0;

	while (true)
	{
		char*current = token[i];
		i += 1;
		if (state < seen_newline && is_newline (current))
		{
			fprintf (ofp, "\n" );
			return true;
		}

		if (state < seen_comment && is_comment (current))
		{
			format_comment (current, column, ofp);
			state = seen_comment;
			continue;
		}

		if (state <= seen_argument
			&& seen_mnemonic_or_declaration <= state
			&& !is_newline (current)
			&& !is_comment (current)
			&& !is_mnemonic_or_declaration (current))
		{
			if (state == seen_mnemonic_or_declaration)
			{
				if (ARGUMENT_COLUMN <= column)
				{
					fprintf(ofp, "\t" );
				} else {
					column += pad_to_column (column, ARGUMENT_COLUMN, ofp);
				}
			}

			if ( !fix_binary_notations (current))
			{
				return false;
			}
			column += format_argument (current, ofp);
			state = seen_argument;
			continue;
		}

		if (state < seen_mnemonic_or_declaration
			&& is_mnemonic_or_declaration (current))
		{
			column += format_mnemonic_or_declaration (current, column, ofp);
			state = seen_mnemonic_or_declaration;
			continue;
		}

		if (state < seen_label
			&& is_macro (current))
		{
			format_comment (current, column, ofp);
			state = seen_comment;
			continue;
		}

		if (state < seen_label
			&& !is_newline (current)
			&& !is_comment (current)
			&& !is_mnemonic_or_declaration (current))
		{
			char*next = token[i];
			bool may_need_newline = !is_newline (next) && !is_comment (next) && !is_declaration (next);
			bool colon_allowed = is_newline (next) || is_mnemonic (next) || is_addressable_declaration (next) || is_comment (next);
			column += format_label (current, may_need_newline, colon_allowed, ofp);
			state = seen_label;
			continue;
		}

		fprintf (stderr, "unexpected token \"%s\" on line %d;" , current, linenr);
		switch (state)
		{
		break; case seen_start:
			fprintf (stderr, "expected label, mnemonic, declaration, or comment.\n");
		break; case seen_label:
			fprintf (stderr, "expected mnemonic, declaration, or comment.\n");
		break; case seen_mnemonic_or_declaration:
		case seen_argument:
			fprintf (stderr, "expected argument or comment\n");
		break; case seen_comment:
			fprintf (stderr, "token parser error\n");
		break; case seen_newline:
		default:
			fprintf (stderr, "token consumer error (seen=%d)\n" , state);
		}
		return false;
	}
}

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

static void freetlist(char**tokens)
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
 * Section: tokenizers
 *
 * grab easily-parsable parts from strings
 */

static size_t wordtokenizer(char const**line, char**token)
{
	while ( **line == ' ' || **line == '\t' )
	{
		*line += 1;
	}

	*token = malloc (strlen ( *line) + 1);

	if ( is_comment ( *line) || is_macro ( *line))
	{
		strcpy ( *token, *line);
		*token = realloc ( *token, strlen ( *token) + 1);
		*line += strlen ( *line);
		return strlen ( *token);
	}

	if ( !isWordChar (true, **line))
	{
		if ( **line && **line != ' ' && **line != '\t' )
		{
			fprintf (stderr, "unexpected character '%c' while looking for label, mnemonic, or declaration\n" , **line );
		}
		free ( *token); *token = 0;
		return 0;
	}

	char*w = *token;
	while (isWordChar (false, **line))
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
	while ( **line == ' ' || **line == '\t' )
	{
		*line += 1;
	}

	*token = malloc (strlen ( *line) + 1);

	if ( is_comment ( *line) || is_macro ( *line))
	{
		strcpy ( *token, *line);
		*token = realloc ( *token, strlen ( *token) + 1);
		*line += strlen ( *line);
		return strlen ( *token);
	}

	char*last_nonwhitespace = *token - 1;
	char*w = *token;
	bool is_in_string = false;
	while (true)
	{
		if ( ! **line)
		{
			break;
		}
		if ( !is_in_string
			&& ( **line == ',' || is_comment ( *line)))
		{
			break;
		}
		if ( **line == '\'' )
		{
			is_in_string = !is_in_string;
		}
		*w = **line;
		if ( **line != ' ' && **line != '\t' )
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

static bool split_line_into_parts(char const*line, char***tokens)
{
	if ( ! line)
	{
		return false;
	}
	char* token;
	size_t ( *tokenizer)(char const**, char** ) = &wordtokenizer;
	while ( *line)
	{
		if ( !tokenizer ( &line, &token))
		{
			free (token); token = 0;
			return !line;
		}
		if (is_mnemonic_or_declaration (token))
		{
			tokenizer = &argtokenizer;
		}
		append (tokens, token);
		free (token); token = 0;
	}
	return true;
}

/******************************************************************************
 * Section: input handlers
 *
 * clean the input and cut into managable blocks
 */

static bool readline(FILE*fp, char**line)
{
	*line = malloc (80);
	char*p = *line;
	char*end_of_buffer = *line + 79;
	char*last_nonwhitespace = p - 1;

	while ( !feof (fp))
	{
		int c = fgetc (fp);
		switch (c)
		{
			case '\r':
			case '':
			case '\0':
				continue;
			break; case EOF:
			case '\n':
				last_nonwhitespace[1] = '\0';
				*line = realloc ( *line, strlen ( *line) + 1);
				return !feof (fp);
		}
		if (c != ' ' && c != '\t' )
		{
			last_nonwhitespace = p;
		}
		*p = c;
		p += 1;
		if (p == end_of_buffer)
		{
			size_t np = p - *line;
			size_t nlnw = last_nonwhitespace - *line;
			*line = realloc ( *line, np + 80);
			p = *line + np;
			end_of_buffer = *line + np + 79;
			last_nonwhitespace = *line + nlnw;
		}
	}
	last_nonwhitespace[1] = '\0';
	return !feof (fp);
}

/******************************************************************************
 * Section: main
 */

int main(int argc, char**argv)
{
	FILE*ifp;
	FILE*ofp;
	char*outfilen = 0;
	if (argc < 2)
	{
		ifp = stdin;
		ofp = stdout;
	} else {
		ifp = fopen (argv[1] , "r" );
		outfilen = malloc (strlen (argv[1] ) + 4 + 1);
		strcpy (outfilen, argv[1] );
		strcat (outfilen, ".frm" );
		ofp = fopen (outfilen, "w" );
	}
	char*input = 0;
	int linenr = 1;
	bool more_lines;
	do
	{
		more_lines = readline (ifp, &input);
		if ( !more_lines && !input[0] ) break;
		char**tokens = malloc (sizeof (char* ));
		*tokens = 0;

		if ( !split_line_into_parts (input, &tokens))
		{
			printf ( "token splitting failed on line %d:\n%s\n" , linenr, input);
			freetlist (tokens); tokens = 0;
			free (outfilen); outfilen = 0;
			return 1;
		}
		if ( !format (tokens, linenr, ofp))
		{
			printf ( "formatting failed on line %d\n%s\n", linenr, input);
			free (input); input = 0;
			freetlist (tokens); tokens = 0;
			free (outfilen); outfilen = 0;
			return 1;
		}
		linenr += 1;

		free (input); input = 0;
		freetlist (tokens); tokens = 0;
	} while (more_lines);
	fclose (ifp);
	fclose (ofp);
	if (outfilen)
	{
		if (rename (outfilen, argv[1] ))
		{
			#include<errno.h>
			char const*err = strerror(errno);
			fprintf (stderr, "could not rename: %s\n" , err);
			free (outfilen); outfilen = 0;
			return 1;
		}
		free (outfilen); outfilen = 0;
	}
	return 0;
}
