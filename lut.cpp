/*		lut.c	Support routines for template class in lut.h

		November 17, 1995	J.A.Lomicka
		June 23, 1998		Christian Baekkelund -- modifications made
*/
#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdio.h>
#include "lut.h"

static char defaultbuffer[256];		// Use this buffer if they don't give us one

const char* sourcelocation(	// Used for throwing filename/linenumber exceptions
	const char* fn,
	int line)
{// See "up" macro in .h file.  "throw up" will throw a file name line number exception
	static char errorstring[ 128];
	strcpy( errorstring, fn);
	strcat( errorstring, " ");
	snprintf( &errorstring[ strlen( errorstring)], 128 - strlen( errorstring), "%d", line);
	return errorstring;
}

const char* abstractlut::namecpy(// Returns a NULL-terminated copy of a name.
	int index,				// Index, must be between count() and count()+place
	char* buf,				// Buffer to put name into
	int blen)				// Size of buffer
{
	int len;			// Length of name
	const char* n;		// Address of name
/*
		This MUST work even if the name contains zeros.  Do NOT use a strncpy
		or strcpy to copy the name!  Although we add an trailing NUL, we can't
		assume the name is a string.  If it is an int or a int, the caller will
		know, and will know the size too.
*/
	if( index > members) return( "");
	if( buf == 0) buf = defaultbuffer;	// Take default if we don't get one
	n = nameptr( &len, index);			// Get address and length of name
	if( len >= blen)
	{// This name won't fit, just deliver what fits and hope it is good enough.
		len = blen-1;
	}
	memcpy( buf, n, len);
	buf[ len] = 0;
	return( buf);
}

const char* abstractlut::displaycpy(// Returns a NULL-terminated copy of a name.
	int index,				// Index, must be between count() and count()+place
	char* buf,				// Buffer to put name into
	int blen)				// Size of buffer
{
	int len;				// Length of name
	const char* n;				// Address of name

	if( buf == 0) buf = defaultbuffer;	// Take default if we don't get one
	n = displayptr( &len, index);		// Get address and length of name
	if( len >= blen)
	{// This name won't fit, just deliver what fits and hope it is good enough.
		len = blen-1;
	}
	memcpy( buf, n, len);
	buf[ len] = 0;
	return( buf);
}

int lutcmp(			// Compare two counted strings, return as per strcmp
	const char* s1,
	int n1,
	const char* s2,
	int n2)
//      Like strcmp(), but uses counted strings, and makes "]" less than digits
{
    int ret;

    for(;;)
    {
        if( n1 == 0) return( -n2);
        if( n2 == 0) return( n1);
        ret = *s1 - *s2;
        if( ret != 0)
    	{ /* Strings are not equal - make ']' come BEFORE anything else */
//        	if( *s1 == ']') return( -1);
//      	if( *s2 == ']') return( +1);
			return( ret);
		}
        n1--; n2--;
        s1++; s2++;
    }
}

int lutncmp(
		const char* s1,
		int n,
		const char* s2,
		int n2)
//      Like strncmp, but limits length of s2 to n2, and gives ']' less than digits
    {
    int ret;

    for(;;)
        {
        if( n == 0) return( 0); /* If S1 runs out first, return equal-to */
        if( n2 == 0) return( n);
        ret = *s1++ - *s2++;
        if( ret != 0)
        	{ /* Strings are not equal - make ']' come BEFORE anything else */
//        	if( *s1 == ']') return( -1);
//        	if( *s2 == ']') return( +1);
			return( ret);
			}
        n--; n2--;
        }
    }

int circmp(	   	/* Compare string to circular buffer */
		const char* s1,			/* Circular buffer */
		int l1,				/* Size of buffer */
		int n1,				/* Insertion point (one past end) */
		const char* s2,			/* String to compare to */
		int n2)				/* Length of string to compare to, MUST BE <= l1 */
	{
	int ret;
	int start;
/*
		We want to compare the last n2 bytes of the circuler buffer, that is
		the most recently inserted n2 bytes, against string s2.
*/
	start = n1-n2;	/* Compute start point */
	if( start < 0)
		{ /* Compare crosses wrap boundary, do in two passes */
		ret = memcmp( &s1[ l1+start], s2, -start);	/* Check front of string */
		if( ret == 0)
			{ /* Not sure yet, check back of string */
			ret = memcmp( s1, &s2[ -start], n2+start);	/* Remember start is negative */
			}
		}
	else
		{ /* Compare does not wrap boundary, do it all at once */
		ret = memcmp( &s1[ start], s2, n2);
		}
	return( ret);
	}

// CB: I had to make this outside of lutclasstest().
struct ltest
{
	const char* name;		// Symbol name
	int value;		// Value
	const char* sname( int *len) { *len = int( strlen( name)); return( name);}
	const char* dname( int *l, int ) {return sname( l);}
	ltest( const char* n, int v) : name( n), value( v) {}
};

void lutclasstest( void)	// test an object
{

	ltest				// Items to use in test
		a( "A", 1),
		b( "B", 2),
		c( "C", 3),
		d( "ED", 4),
		e( "EE", 5);
	lut<ltest> l;		// List to test
	ltest *z;			// Temporary
	
	z=l.first();
	if( z != NULL)
	{ // This test makes sure empty lists are empty
		throw "lut 1";
    }
	z = l.lookup( "Q", 1);	// Make sure lookups on empty are okay
	if( z) throw "lut 1a";
//		Test order of list

	l.selftest();
	l.add( &e);
	l.add( &a);
	l.add( &c);
	l.selftest();
	z = l.first();
	if( z->value != 1) throw "lut 2";
	z = l.next( z);	
	if( z->value != 3) throw "lut 3";	
	z = l.next( z);	
	if( z->value != 5) throw "lut 4";	
	z = l.next( z);	
	if( z) {throw "lut 5";}	// Sould be null
	if( !l.remove( &c)) throw "lut 5a";			// test remove
	if( l.remove( &c)) throw "lut 5b";			// test remove
	l.selftest();
	z = l.first();
	if( z->value != 1) throw "lut 7";
	z = l.next( z);	
	if( z->value != 5) throw "lut 8";	
	z = l.next( z);	
	if( z) throw "lut 9";	// Sould be null
	l.add( &c);
	l.add( &b);
	l.add( &d);
	int count;
	z = l.lookup_a( "", 0, count);
	if( z->value != 1) throw "lut 10";
	if( count != 5) throw "lut 11";
	z = l.lookup_a( "E", 1, count);
	if( count != 2) throw "lut 12";
	if( z->value != 4) throw "lut 13";
	for( z=l.first(); z!= NULL; z=l.next( z))
		if( !l.remove( z)) throw "lut 14";			// Empty the list
	if( l.first() != NULL) throw "lut 15";
}

/*		End of lut.c */
