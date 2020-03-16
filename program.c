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
#	define COMMENT_COLUMN (ARGUMENT_COLUMN + (3 * INDENT_WIDTH))
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
#	define	INDENT_CHAR '\t'
#	define	INDENT_STR "\t"
#else
#	define	INDENT_CHAR ' '
#	define	INDENT_STR " "
#endif

/******************************************************************************
 * Section: formatter helpers
 */

static int
insert_semantic_whitespace(int column, FILE*ofp)
{
	fprintf (ofp, INDENT_STR);
	#if defined (ONLY_TABS)
	return TAB_WIDTH - (column % TAB_WIDTH);
	#else
	return 1;
	#endif
}

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
			fprintf (ofp, INDENT_STR );
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

static int format_mnemonic_or_declaration (char const*token, FILE*ofp)
{
	char*lowercase_token = malloc(strlen (token) + 1);
	lowercase_string(lowercase_token, token);

	int chars_printed = fprintf (ofp, "%s" , lowercase_token);
	free (lowercase_token); lowercase_token = 0;
	return chars_printed;
}

static int format_argument (char const*token, FILE*ofp)
{
	// TODO: parens around negative numbers

	char*ntoken = strdup (token);
	fix_binary_notations (ntoken);
	normalize_words_in (ntoken);

	int printed = fprintf (ofp, "%s", ntoken);
	free (ntoken); ntoken = 0;
	return printed;
}

static int format_label (char const*token, FILE*ofp)
{
	char*ntoken = strdup (token);
	normalize_label (ntoken, token);

	int column = fprintf (ofp, "%s", token);

	free (ntoken); ntoken = 0;
	return column;
}

#include"libasm8051/delegate.h"

/******************************************************************************
 * Section: formatting dispatcher
 *
 * delegate control to token formatter
 */
bool format_line(struct addressed const*a, void*data)
{
	FILE*ofp = *(FILE**)data;
	int column = 0;
	if (a->label)
	{
		column += format_label (a->label[0], ofp);
		if ( a->mnemonic_or_declaration
			&& !is_mnemonic (a->mnemonic_or_declaration)
			&& !is_addressable_declaration (a->mnemonic_or_declaration))
		{
			column += insert_semantic_whitespace (column, ofp);
		} else {
			if (a->label[0][strlen (a->label[0]) - 1] != ':' && !is_segment_label (a->label[0]))
			{
				column += fprintf (ofp, "%c", ':');
			}
			if ((is_mnemonic (a->mnemonic_or_declaration) && MNEMONIC_COLUMN < column)
				|| (is_addressable_declaration(a->mnemonic_or_declaration) && DECLARATOR_COLUMN < column))
			{
				fprintf (ofp, "\n");
				column = 0;
			}
		}
	}
	if (a->mnemonic_or_declaration)
	{
		if (is_macro (a->mnemonic_or_declaration))
		{
			column += fprintf (ofp, "%s", a->mnemonic_or_declaration);
		} else {
			if (is_mnemonic (a->mnemonic_or_declaration)
				&& column < MNEMONIC_COLUMN)
			{
				column += pad_to_column (column, MNEMONIC_COLUMN, ofp);
			}
			else if (is_declaration (a->mnemonic_or_declaration)
				&& column < DECLARATOR_COLUMN)
			{
				column += pad_to_column (column, DECLARATOR_COLUMN, ofp);
			} else if (column < MNEMONIC_COLUMN){
				column += pad_to_column (column, MNEMONIC_COLUMN, ofp);
			}
			column += format_mnemonic_or_declaration (a->mnemonic_or_declaration, ofp);
		}
	}
	if (a->argument)
	{
		column += insert_semantic_whitespace (column, ofp);
		if (column < ARGUMENT_COLUMN)
		{
			column += pad_to_column (column, ARGUMENT_COLUMN, ofp);
		}
		char*seperator = "";
		for (int i = 0; a->argument[i]; i += 1)
		{
			column += fprintf (ofp, "%s", seperator);
			column += format_argument (a->argument[i], ofp);
			seperator = ", ";
		}
	}
	if (a->comment)
	{
		if (column && column < COMMENT_COLUMN)
		{
			pad_to_column (column, COMMENT_COLUMN, ofp);
		}
		fprintf (ofp, "%s", a->comment[0]);
	}
	fprintf (ofp, "\n");
	return true;
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
	callback_lexed_line (ifp, &format_line, &ofp);
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
