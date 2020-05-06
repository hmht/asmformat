#include"infix.h"
#include<stddef.h>
#include<stdbool.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<stdio.h>

#include"syntax.h"

static bool
is_whitespace (char const c)
{
	return c == ' ' || c == '\t';
}

static bool
is_binary_operator (char const c)
{
	return c == '+' || c == '-' || c == '/' || c == '*';
}

size_t
tokenize_expression(char const**e, char**token)
{
	char*w=*token;
	while (is_whitespace (**e))
	{
		*e += 1;
	}

	if ( is_binary_operator (**e)
		|| '(' == **e
		|| ')' == **e
	) {
		*w = **e;
		w += 1;
		*e += 1;
	}
	else if (opens_string (**e))
	{
		while (**e && !closes_string (*(*e - 1), **e))
		{
			*w = **e;
			w += 1;
			*e += 1;
		}
	}
	else
	{
		while ( **e
			&& !is_binary_operator (**e)
			&& '(' != **e
			&& ')' != **e
			&& !opens_string (**e)
			&& !is_whitespace (**e))
		{
			*w = **e;
			w += 1;
			*e += 1;
		}
	}
	*w = '\0';
	*token = realloc (*token, strlen (*token) + 1);
	return strlen (*token);
}

static void
full_tokenize_expression(char const*e, char***token)
{
	while (e && *e)
	{
		char*new = malloc (strlen (e) + 1);
		tokenize_expression (&e, &new);
		if (*new)
		{
			#include"strarray.h"
			strarray_append (token, new);
			free (new); new = 0;
		}
		else
		{
			free (new); new = 0;
			return;
		}
	}
}

static uint8_t
nibble(char n, struct parseerror*e)
{
	if ('0' <= n && n <= '9')
	{
		return n - '0';
	}
	if ('a' <= n && n <= 'f')
	{
		return n - 'a' + 10;
	}
	if ('A' <= n && n <= 'F')
	{
		return n - 'A' + 10;
	}
	e->at = 0;
	e->expected = "0-9, a-f, A-F";
	e->got = malloc (2);
	sprintf( e->got, "%c", n);
	return 0;
}

static uint16_t
resolve_hexadecimal(char const*h, struct parseerror*e)
{
	if (!h[0])
	{
		e->at = 0;
		e->expected = "hexadecimal number";
		e->got = strdup ("nothing");
		return 0;
	}
	uint16_t out = 0;
	for (int i = 0; h[i]; i += 1)
	{
		struct parseerror ne = {0};
		out *= 0x10;
		out += nibble (h[i], &ne);
		if (4 <= i)
		{
			e->at = i;
			e->expected = "8 or 16-bit hex number";
			e->got = strdup ("more than 4 hex digits");
			return 0;
		}
		if (ne.got)
		{
			e->at = i;
			e->expected = ne.expected;
			e->got = ne.got;
			return 0;
		}
	}
	return out;
}

static uint16_t
resolve_binary(char const*b, struct parseerror*e)
{
	if (!b[0])
	{
		e->at = 0;
		e->expected = "binary number";
		e->got = strdup ("nothing");
		return 0;
	}
	uint16_t out = 0;
	for (int i = 0; b[i]; i += 1)
	{
		out *= 2;
		if (b[i] == 'b' || b[i] == 'B')
		{
			break;
		}
		if (b[i] == '1')
		{
			out += 1;
		}
		else if (b[i] != '0')
		{
			e->at = i;
			e->expected = "binary number of 0s and 1s";
			e->got = malloc (2);
			sprintf (e->got, "%c", b[i]);
			return 0;
		}
		if (16 <= i)
		{
			e->at = i;
			e->expected = "16-bit binary number";
			e->got = strdup ("more than 16 bits");
			return 0;
		}
	}
	return out;
}

static uint16_t
resolve_decimal(char const*s, struct parseerror*e)
{
	uint16_t v = 0;
	if(!*s)
	{
		e->at = 0;
		e->expected = "decimal number";
		e->got = "nothing";
		return 0;
	}
	int i = 0;
	while(s[i])
	{
		v *= 10;
		if (s[i] < '0' || s[i] > '9')
		{
			e->at = i;
			e->expected = "decimal digits 0-9";
			e->got = malloc (2);
			sprintf (e->got, "%c", s[i]);
			return 0;
		}
		v += s[i] - '0';
		i += 1;
		// TODO: check 16-bit?
	}
	return v;

}

static uint16_t
resolve_string(char const*w, struct parseerror*e)
{
	if (!w[0])
	{
		e->at = 0;
		e->expected = "character or string";
		e->got = strdup ("nothing");
		return 0;
	}
	bool expect_close = opens_string (w[0]);
	off_t i = expect_close;

	if((i && closes_string (w[i], w[0])) || !w[i])
	{
		e->at = i;
		e->expected = "character or string";
		e->got = strdup ( expect_close ? "stray quotes" : "empty string");
		return 0;
	}

	uint16_t val = w[i];
	i += 1;
	(void) val;

	e->at = 0;
	e->expected = "decimal or hexadecimal";
	e->got = strdup ("character or string (not yet supported");
	return 0;
}

static bool
looks_like_binary(char const*w)
{
	int i = 0;
	while (w && w[i])
	{
		if (w[i] == '%')
		{
			continue;
		}
		if (w[i] == 'b' || w[i] == 'B')
		{
			return i && !w[i + 1];
		}
		if (w[i] != '0' && w[i] != '1' && w[i] != '_')
		{
			return false;
		}
		i += 1;
	}
	return w && w[0] == '%';
}

static bool
looks_like_hexadecimal(char const*w)
{
	int i = 0;
	if (!w)
	{
		return false;
	}
	if (w[0] == '$')
	{
		i = 1;
	}
	if (w[0] == '0' && (w[1] == 'x' || w[1] == 'X'))
	{
		i = 2;
	}
	if (!i)
	{
		return false;
	}
	while (w && w[i])
	{
		if (w[i] == 'h' || w[i] == 'H')
		{
			return i && !w[i + 1];
		}
		if (w[i] == '_')
		{
			continue;
		}
		if ( (w[i] < '0' || '9' < w[i])
			&& (w[i] < 'a' || 'f' < w[i])
			&& (w[i] < 'A' || 'F' < w[i]))
		{
			return false;
		}
		i += 1;
	}
	return true;
}

static void
extract_symbol(char const*w, char**sym, char**bit)
{
	char*dot = strchr (w, '.');
	if (dot)
	{
		*bit = malloc (strlen (dot+1) + 1);
		strcpy (*bit, dot+1);
		*sym = malloc ((dot - w) + 1);
		strncpy (*sym, w, (dot - w));
		(*sym)[dot-w] = '\0';
	}
	else
	{
		*sym = strdup (w);
	}
}

static uint16_t
resolve_word(char const*w, struct trie*symbol, struct parseerror*e)
{
	if (looks_like_hexadecimal (w))
	{
		uint16_t out = resolve_hexadecimal (w + 1, e);
		e->at += 1;
		return out;
	}
	if (looks_like_binary (w))
	{
		uint16_t out = resolve_binary (w + 1, e);
		e->at += 1;
		return out;
	}
	if (opens_string (*w))
	{
		return resolve_string (w, e);
	}
	if ('0' <= *w && *w <= '9')
	{
		return resolve_decimal (w, e);
	}
	char*sym = 0;
	char*bitstr = 0;
	extract_symbol(w, &sym, &bitstr);
	uint16_t*v = trie_lookup (symbol, sym);
	free (bitstr), bitstr = 0;
	if (!v)
	{
		e->at = 0;
		e->expected = "previously-defined symbol or constant";
		e->got = strdup (sym);
		free (sym), sym = 0;
		return 0;
	} else {
		// TODO: bit-in-byte declarations like acc.7
		free (sym), sym = 0;
		return *v;
	}
}

struct sexpr // not an s-expression at all.
{

	enum operator
	{
		op_value,
		op_negative,
		op_high,
		op_low,
		op_subtract,
		op_add,
		op_divide,
		op_multiply,
		op_remainder,
	} operator;
	struct sexpr_arg {
		enum argtype {
			arg_null,
			arg_subexpr,
			arg_word,
			arg_string,
		} type;
		union {
			struct sexpr*sub;
			char*word;
		};
	} arg[2];
};

static void
sexpr_free(struct sexpr*s)
{
	if (s->arg[0].type == arg_subexpr)
	{
		sexpr_free (s->arg[0].sub); s->arg[0].sub = 0;
	}
	if (s->arg[1].type == arg_subexpr)
	{
		sexpr_free (s->arg[1].sub); s->arg[1].sub = 0;
	}
	if (s->arg[0].type == arg_word
		|| s->arg[0].type == arg_string)
	{
		free (s->arg[0].word); s->arg[0].word = 0;
	}
	if (s->arg[1].type == arg_word
		|| s->arg[1].type == arg_string)
	{
		free (s->arg[1].word); s->arg[1].word = 0;
	}
	free (s); s = 0;
}

static enum operator
get_operator(char const*op)
{
	switch (*op)
	{
	break; case'-':
		return op_negative;
	break; case'+':
		return op_add;
	break; case'*':
		return op_multiply;
	break; case'/':
		return op_divide;
	}
	if (!strcasecmp (op, "low"))
	{
		return op_low;
	}
	if (!strcasecmp (op, "high"))
	{
		return op_high;
	}
	return op_value;
}

char*describe_operator[] = {
	[op_value] = "",
	[op_negative] = "-",
	[op_subtract] = "-",
	[op_add] = "+",
	[op_divide] = "/",
	[op_multiply] = "*",
	[op_high] = "high",
	[op_low] = "low",
};

static void
sexpr(char**token, struct sexpr*s, struct parseerror*e)
{
	int paren_nest_depth = 0;
	char**parenstart = 0;
	for (int i = 0; token[i]; i += 1)
	{
		if (*token[i] == '(')
		{
			paren_nest_depth += 1;
			if (!parenstart)
			{
				parenstart = &token[i];
			}
			continue;
		}
		struct sexpr_arg*a;
		if(!s->arg[0].type)
		{
			a = &s->arg[0];
		}
		else if(!s->arg[1].type)
		{
			a = &s->arg[1];
		}
		else
		{
			struct sexpr*sub = malloc (sizeof*sub);
			sub->operator = s->operator;
			memcpy (&sub->arg[0], &s->arg[0], sizeof s->arg[0]);
			memcpy (&sub->arg[1], &s->arg[1], sizeof s->arg[1]);

			s->arg[0].type = arg_subexpr;
			s->arg[0].sub = sub;
			s->operator = op_value;
			s->arg[1].type = arg_null;

		}
		if (!parenstart)
		{
			if (opens_string (*token[i]))
			{
				a->type = arg_string;
				a->word = strdup (token[i]);
				continue;
			}
			enum operator operator = get_operator (token[i]);
			if(operator)
			{
				if (s->operator != op_value)
				{
					e->at = i;
					e->expected = "variable, constant, or parenthesised expression";
					const char errmsg[] = "\"%s %s\" <- double operator";
					e->got = malloc (sizeof (errmsg)
						       + strlen (describe_operator[s->operator])
						       + strlen (token[i]));
					sprintf (e->got, errmsg, describe_operator[s->operator], token[i]);
					return;
				}
				if (a->type && operator == op_negative)
				{
					operator = op_subtract;
				}
				s->operator = operator;
				continue;
			}
			a->type = arg_word;
			a->word = strdup (token[i]);
			continue;
		}
		if (*token[i] == ')')
		{
			paren_nest_depth -= 1;
			if(!paren_nest_depth)
			{
				#include"strarray.h"
				int sublen = &token[i - 1] - &parenstart[1] + 1;
				char**sub = malloc (sizeof (*sub) * (sublen + 1));
				memcpy (sub, &parenstart[1], sizeof (*sub) * sublen);
				sub[sublen] = 0;
				parenstart = 0;

				a->type = arg_subexpr;
				a->sub = malloc (sizeof (*a->sub));
				a->sub->operator = op_value;
				a->sub->arg[0].type = arg_null;
				a->sub->arg[1].type = arg_null;
				sexpr (sub, a->sub, e);
				free (sub); sub = 0;
				continue;
			}
		}
	}
}

static uint16_t solve_sexpr(struct sexpr const*, struct trie*sym, struct parseerror*);

static uint16_t
solve_sexpr_arg(struct sexpr_arg const*a, struct trie*sym, struct parseerror*e)
{
	switch (a->type)
	{
	break; case arg_null:
		e->at = 0;
		e->expected = "argument";
		e->got = strdup ("nothing");
		return 0;
	break; case arg_subexpr:
		return solve_sexpr (a->sub, sym, e);
	break; case arg_word:
	case arg_string:
		return resolve_word (a->word, sym, e);
	}
}

static uint16_t
solve_sexpr(struct sexpr const*s, struct trie*sym, struct parseerror*e)
{
	switch (s->operator)
	{
	break; case op_value:
		return solve_sexpr_arg (s->arg, sym, e);
	break; case op_negative:
		return -solve_sexpr_arg (s->arg, sym, e);
	break; case op_high:
		return ( solve_sexpr_arg (s->arg, sym, e) & 0x0FF00 ) >> 8;
	break; case op_low:
		return solve_sexpr_arg (s->arg, sym, e) & 0x00FF;
	break; case op_subtract:
		return solve_sexpr_arg (s->arg, sym, e) - solve_sexpr_arg (&s->arg[1], sym, e);
	break; case op_add:
		return solve_sexpr_arg (s->arg, sym, e) + solve_sexpr_arg (&s->arg[1], sym, e);
	break; case op_divide:
		return solve_sexpr_arg (s->arg, sym, e) / solve_sexpr_arg (&s->arg[1], sym, e);
	break; case op_multiply:
		return solve_sexpr_arg (s->arg, sym, e) * solve_sexpr_arg (&s->arg[1], sym, e);
	break; case op_remainder:
		return solve_sexpr_arg (s->arg, sym, e) % solve_sexpr_arg (&s->arg[1], sym, e);
	}
}

extern uint16_t
resolve_expression(char const*e, struct trie*sym, struct parseerror*err)
{
	#include"strarray.h"
	char**token = 0;
	full_tokenize_expression (e, &token);
	struct sexpr*s = malloc (sizeof (*s));
	s->arg[0].type = arg_null;
	s->arg[1].type = arg_null;
	s->operator = op_value;
	sexpr(token, s, err);
	strarray_free (token); token = 0;
	uint16_t v = solve_sexpr(s, sym, err);
	sexpr_free (s); s = 0;
	return v;
}

/*
int main(int argc, char**argv)
{
	struct trie sym = {0};
	struct parseerror e = {0};
	printf( "%s = %d\n", argv[1], resolve_expression (argv[1], &sym, &e));
	if (e.got)
	{
		fprintf (stderr, "expected: %s\ngot: %s\n", e.expected, e.got);
		free (e.got); e.got = 0;
	}
	return 0;
}
*/
