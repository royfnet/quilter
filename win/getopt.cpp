/* "Expat License" (https://directory.fsf.org/wiki/License:Expat) */

/* Copyright 2016 Roy Lomicka
 
Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:
 
The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */ 

/*	derived from AT&T public-domain getopt published in mod.sources
**	(i.e., comp.sources.unix before the great Usenet renaming) */

#define _CRT_NONSTDC_NO_WARNINGS

#include <string.h>
#include <io.h>

#define _NUL_ ('\0')
#define TRUE 1
#define FALSE 0

#define ERR(s, c)\
	if (opterr)\
	{	/* - */\
		char buff[3];\
		\
		buff[0] = (char) c; buff[1] = '\r'; buff[2] = '\n';\
		(void) write(2, av[0], (unsigned int) strlen(av[0]));\
		(void) write(2, s, (unsigned int) strlen(s));\
		(void) write(2, buff, 3);\
	}\

int opterr = TRUE;
	/*- implicit input to getopt();
	** TRUE to allow getopt() to log errors to stderr;
	** FALSE to suppress logging of errors by getopt() */
int optind = 1;
	/*- implicit input to and output from getopt();
	** index of next element of av to be processed */
int optopt = 0;
	/*- implicit output from getopt(); option character */
const char * optarg = (char *) NULL;
	/*- implicit output from getopt(); option argument */

/* internal context of getopt(); implicit input and output */
static int i = 1; /*- first option character is at beginning of next element of av */
	/*- position of next option character within element of av */

int optreset = 0;

/* not thread-safe; typically call from main thread */
int getopt(
	/* Get option.
	**
	** Set optopt to option character processed. Update optarg and optind
	** as follows: If option requires argument (option character was followed
	** by colon in opts), set optarg to point to option argument as follows: 
	** If option was last character in element of av, set optarg to point to
	** next element of av, and increment optind by 2. If resulting value of
	** optind is not less than argc, return '?'. (This indicates missing option
	** argument.)  Otherwise, set optarg to point to string following option
	** character in element of av, and increment optind by 1. */
	int ac,
		/*- argument count */
	const char ** av,
		/*- arguments */
	const char * opts
		/*- string of option characters with colon following
		** each option character that takes an argument */
	/* Return optopt if option character in element of av
	** matches an option character in opts and if option
	** requires argument, option argument is not missing.
	**
	** Return -1 if av[optind][0] is not '-' (done).
	** Return -1 if av[optind] is "--" (done).
	** (Option processing stops as soon as a non-option argument is encountered.)
	**
	** Return '?' if option argument is missing (failure).
	** Return '?' if optopt is not specified in opts (failure). */
){
	if (optreset)
	{// reset for rescan
		opterr = TRUE;
		optind = 1;
		optopt = 0;
		optarg = (char *)NULL;
		i = 1;
		optreset = 0;
	}
	if (i == 1)
	{	/* at beginning of element - see if done */
		if (
			optind >= ac ||
			(av[optind][0] != '-' && av[optind][0] != '/') ||
			av[optind][1] == _NUL_
		)
		{	/* av[optind][0] is not '-' (element of av does not contain option characters) - done (do not increment optind) */
			return (-1);
		}
		if (strcmp(av[optind], "--") == 0)
		{	/* av[optind] is "--" - update optind; done */
			optind++;
			return (-1);
		}
	}
	/* not done */

	/* get next option */
	optopt = av[optind][i];
	{char * p; if (optopt == ':')
	{	/* colon - not an option */
		p = (char *) NULL;
	}
	else
	{	/* not colon - see if in list */
		p = (char*)strchr(opts, optopt);
	}
	if ((char *) NULL == p)
	{	/* optopt not in opts - prepare for next; fail */
		ERR(": unsupported option -- ", optopt);
		i++; /*- next character in current element */
		if (_NUL_ == av[optind][i])
		{	/* end of element - prepare to process next element */
			optind++;
			/* next option character is at beginning of next element of av */
			i = 1;
		}
		return ('?');
	}
	if (p[1] == ':')
	{	/* option requires argument - */
		if (_NUL_ != av[optind][i + 1])
		{	/* argument follows option in element - point to argument; succeed */
			optarg = &av[optind][i + 1];
			optind++;
		}
		else
		{	/* argument is next element - prepare for next */
			optind++;
			if (optind >= ac)
			{	/* no next element - fail */
				ERR(": option requires an argument -- ", optopt);
				/* next option character is at beginning of next element of av */
				i = 1;
				return ('?');
			}
			/* point to argument - succeed */
			optarg = av[optind];
			optind++;
		}
		/* next option character is at beginning of next element of av */
		i = 1;
	}
	else
	{	/* option does not take argument - prepare for next; succeed */
		i++; /*- next character in current element */
		if (av[optind][i] == _NUL_)
		{	/* end of element - prepare to process next element */
			optind++;
			/* next option character is at beginning of next element of av */
			i = 1;
		}
		optarg = (char *) NULL;
	}}

	return (optopt);
}
