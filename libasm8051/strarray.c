#include"strarray.h"
#include<stddef.h>
#include<stdlib.h>
#include<string.h>
extern void strarray_append(char***array, char const*value)
{
	size_t nvalues = 0;
	while ( *array && ( *array)[nvalues] )
	{
		nvalues += 1;
	}
	nvalues += 1;
	*array = realloc ( *array, (nvalues + 1) * sizeof (char** ));
	( *array)[nvalues - 1] = strdup (value);
	( *array)[nvalues] = 0;
}

extern void strarray_free(char**array)
{
	if ( !array)
	{
		return;
	}
	size_t i = 0;
	while (array[i] )
	{
		free (array[i] ); array[i] = 0;
		i += 1;
	}
	free (array); array = 0; (void)array;
}

extern void
strarray_merge(char***dest, char**src)
{
	int i = 0;
	if (!*dest)
	{
		*dest = src;
	} else {
		#include"strarray.h"
		while (src && src[i]) {
			strarray_append (dest, src[i]);
			i += 1;
		}
		strarray_free (src); src = 0;
		free (src); src = 0;
	}
}
