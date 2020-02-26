#include"syntax.h"
#include<strings.h>
extern bool is_allcaps(char const*token)
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

extern bool is_newline(char const*token)
{
	return !token;
}

extern bool is_comment(char const*token)
{
	if ( !token)
	{
		return false;
	}
	return token[0] == ';' ;
}

extern bool is_macro(char const*token)
{
	if ( !token)
	{
		return false;
	}
	return token[0] == '$' ;
}

extern bool is_segment_label(char const*token)
{
	if ( !token)
	{
		return false;
	}
	static char const*const segment_label[] = {
		"xseg",
		"cseg",
		0
	};
	for (char const*const*p = segment_label ; *p ; p += 1)
	{
		if (!strcasecmp ( *p, token))
		{
			return true;
		}
	}
	return false;
}

extern bool is_addressable_declaration(char const*token)
{
	if ( !token)
	{
		return false;
	}
	static char const*const addressable_declarator[] = {
		"include",
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

extern bool is_declaration(char const*token)
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
		"bit",
		"data",
		"xdata",
		"equ",
		"code",
		"org",
		"public",
		"extern",
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

extern bool is_mnemonic(char const*token)
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

extern bool is_mnemonic_or_declaration(char const*token)
{
	if ( !token)
	{
		return false;
	}
	return is_declaration (token)
		|| is_mnemonic (token)
		|| !strcasecmp (token, "end");
}

extern bool has_arguments(char const*token)
{
	if ( !token)
	{
		return false;
	}
	return is_mnemonic_or_declaration(token);
}

extern bool is_word_char(bool first, char c)
{
	return ( 'A' <= c && c <= 'Z' )
		|| ( 'a' <= c && c <= 'z' )
		|| c == '_' || c == '?'
		|| (first? false : ( '0' <= c && c <= '9' ));
}

extern bool has_blob(char const*token)
{
	return token[0] == '$';
}

extern bool opens_string(char c)
{
	return c == '\'';
}

extern bool closes_string(char prev, char next)
{
	return next == '\'' && prev != '\\';
}

