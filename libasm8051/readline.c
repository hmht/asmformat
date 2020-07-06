#include"readline.h"
#include<stdlib.h>
#include<string.h>
#include<stddef.h>
extern bool readline(FILE*fp, char**line)
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
			case 0x1a:
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
			int np = p - *line;
			int nlnw = last_nonwhitespace - *line;
			*line = realloc ( *line, (np + 80) * sizeof**line);
			p = *line + np;
			end_of_buffer = *line + np + 79;
			last_nonwhitespace = *line + nlnw;
		}
	}
	last_nonwhitespace[1] = '\0';
	return !feof (fp);
}

