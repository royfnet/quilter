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

#ifndef _GETOPT_H_
#define _GETOPT_H_

extern int opterr;
/*- implicit input to getopt();
** TRUE to allow getopt() to log errors to stderr;
** FALSE to suppress logging of errors by getopt() */
extern int optind;
/*- implicit input to and output from getopt();
** index of next element of av to be processed */
extern int optopt;
/*- implicit output from getopt(); option character */
extern const char * optarg;
/*- implicit output from getopt(); option argument */

extern int optreset;	// Just for Mac compatibility

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
);

#endif
