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
#include<unistd.h>

#include"libasm8051/token.h"
#include"libasm8051/syntax.h"

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
 * Section: formatter helpers
 */

static int pad_to_column(int from, int to, FILE*ofp)
{
	if (from < to)
	{
		int column_diff = to-from;
		for (int needed_spaces = column_diff
			#if defined(ONLY_TABS)
			/ INDENT_WIDTH + ( !! (column_diff % INDENT_WIDTH))
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
	char*word = malloc(strlen (string_out) + 1);
	char*wp = word;
	char*string_in = string_out;
	bool is_in_string = false;
	while (*string_in)
	{
		if ( *string_in == '\'' )
		{
			is_in_string = !is_in_string;
		}
		if ( !is_in_string && is_word_char (wp == word, *string_in))
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
	free (word); word = 0;
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
	int chars_printed = pad_to_column (column, min_column, ofp);

	char*lowercase_token = malloc(strlen (token) + 1);
	lowercase_string(lowercase_token, token);

	chars_printed += fprintf (ofp, "%s" , lowercase_token);
	if ( is_declaration (token) )
	{
		fprintf (ofp, "%c",
			#ifdef ONLY_TABS
			'\t'
			#else
			' '
			#endif
			);
		chars_printed += 1
			#ifdef ONLY_TABS
			* TAB_WIDTH
			#endif
			;
	}
	free (lowercase_token); lowercase_token = 0;
	return chars_printed;
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
					column += (fprintf (ofp,
					#ifdef ONLY_TABS
					"\t" ), column % 8);
					#else
					" " ));
					#endif
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
			fprintf (stderr, "expected label, mnemonic, declaration, or comment as first token.\n");
		break; case seen_label:
			fprintf (stderr, "expected mnemonic, declaration, or comment after a label (\"%s\").\n", current-1);
		break; case seen_mnemonic_or_declaration:
			fprintf (stderr, "expected argument or comment after mnemonic/declaration \"%s\"\n", current-1);
		break; case seen_argument:
			fprintf (stderr, "expected argument or comment after argument \"%s\"\n", current-1);
		break; case seen_comment:
			fprintf (stderr, "stray token after comment \"%s\"\n", current-1);
		break; case seen_newline:
		default:
			fprintf (stderr, "token consumer error (seen=%d)\n" , state);
		}
		return false;
	}
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
		if (!ifp) {
			fprintf (stderr, "could not open %s\n", argv[1]);
			return 1;
		}
		outfilen = malloc (strlen (argv[1] ) + 4 + 1);
		strcpy (outfilen, argv[1] );
		strcat (outfilen, ".frm" );
		// guard for overwriting file?
		ofp = fopen (outfilen, "w" );
		if (!ofp) {
			fprintf (stderr, "could not open %s\n", outfilen);
			fclose (ifp); ifp = 0;
			free (outfilen); outfilen = 0;
			return 1;
		}
	}
	int linenr = 1;
	bool more_lines;
	do
	{
		#include"libasm8051/readline.h"
		#include"libasm8051/strarray.h"
		char*input = 0;
		more_lines = readline (ifp, &input);
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
			free (outfilen); outfilen = 0;
			return 1;
		}
		if ( !format (tokens, linenr, ofp))
		{
			fprintf (stderr, "formatting failed on line %d\n%s\n", linenr, input);
			free (input); input = 0;
			strarray_free (tokens); tokens = 0;
			free (outfilen); outfilen = 0;
			return 1;
		}
		linenr += 1;

		free (input); input = 0;
		strarray_free (tokens); tokens = 0;
	} while (more_lines);
	fclose (ifp); ifp = 0;
	fclose (ofp); ofp = 0;
	if (outfilen)
	{
		unlink(argv[1]);
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
