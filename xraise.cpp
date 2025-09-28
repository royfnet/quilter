/*		xraise.c		an xraise package

		Implements a primitive xraise exception.  Note it doesn't implement any
		sort of xprotect/xexception stuff, just xraise.  You catch them like this:
		
			try
			{
				...whatever...
			}
			catch( std::exception& err)
			{
				DisplayException( err);
			}
*/

#include "xraise.h"
#include <cstdarg>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

//		Useful utilities for which this is a good place to put them

bool noisy = false;		// Debugging logs

bool frontcmp( const char* pattern, const char* s)
{
	size_t len = strlen( pattern);
	return 0 == strncmp( pattern, s, len);
}

char tolower_c( char ch)
{// Type-matched to char instead of int
	return (char) tolower( ch);
}

char toupper_c( char ch)
{// Type-matched to char instead of int
	return (char) toupper( ch);
}

void EndianSwap( void* thing, size_t size)
{// Variable sized endian swapper for any data type
	char* t = (char*) thing;
	for( size_t i = 0; i < size/2; ++i )
	{
		char temp = t[ i];
		t[ i] = t[ size-i-1];
		t[ size-i-1] = temp;
	}
}

int strli( const char* s)		// Integer version of strlen
{
	return (int) strlen( s);
}

const char* ctim( time_t tv)
{// essentially ctime() without the stupid terminating newline
 // but also uses a 4-entry ring buffer so you can have four simultaniously active time buffers
 // Still not thread safe 'cause sTimeBufPos is not atomic.
	static char sTimeBuffer[ 4][26];
	static int sTimeBufPos = 0;
#if WINCODE
	char* retval = sTimeBuffer[sTimeBufPos];
	ctime_s(sTimeBuffer[sTimeBufPos++], 26, &tv);
#else
    char* retval = ctime_r( &tv, sTimeBuffer[sTimeBufPos++]);
#endif
    Test( retval);
    retval[ strlen( retval)-1]=0;
    sTimeBufPos &= 3;
    return retval;
}


//		Actual exeption related stuff

void exexception::SetFileLine( const char* file, int line) const
{
    const char* filename = strrchr( file, '/');     // Strip off path, if there is one
    if( filename == nullptr)
        filename = file;    // There was no path
    else
        ++filename;         // There was a path, skip over the slash
	snprintf( whatString, CountItems( whatString), "%s, file %s line %d", whatString, filename, line);
}

exexception::exexception( const char* s)
{
	strcpy( whatString, s);
}

exexception::exexception( std::exception* eek)
{
	strcpy( whatString, eek->what());
}

const char* exexception::what() const throw()
{
	return whatString;
}

exexception xraiseit( const char* code, ...)
{
	va_list		args;
	char message[ 1000];
	char* argType;
	char* pos = message;
	#define rem (CountItems( message) - int(pos - message))
	// Add parameters to the error from the variable argument list.
	va_start(args, code);
	pos += snprintf( pos, rem, "%s", code);
	for(;;)
	{// Append argument list to the exception string
		argType = va_arg( args, char*);
		if( argType == NULL)
			break;
		if( frontcmp( "str ", argType))
		{
			char* value = va_arg( args, char*);
			pos = pos + snprintf( pos, rem, "~ %s=%s", &argType[ 4], value);
		}
		if( frontcmp( "int ", argType))
		{
			int ivalue = va_arg( args, int);
			pos = pos + snprintf( pos, rem, "~ %s=%d", &argType[ 4], ivalue);
		}
		if( frontcmp( "hex ", argType))
		{
			int ivalue = va_arg( args, int);
			pos = pos + snprintf( pos, rem, "~ %s=%x", &argType[ 4], ivalue);
		}
		if( frontcmp( "int64 ", argType))
		{
			int64 ivalue = va_arg( args, int64);
			pos = pos + snprintf( pos, rem, "~ %s=%lld", &argType[ 4], ivalue);
		}
		if( frontcmp( "double ", argType))
		{
			double dvalue = va_arg( args, int);
			pos += snprintf( pos, rem, "~ %s=%f", &argType[ 7], dvalue);
		}
	}
	va_end(args);
	return exexception( message);
}

void operator+( const FileLine& f, const exexception& e)
{
	const char* file = strrchr( f.iFile, '\\');
	if( file == NULL)
		file = f.iFile;
	else
		file++;
	e.SetFileLine( file, f.iLine);
	throw e;
}

bool operator==( const std::exception& e, const char* s)
{
	return strncmp( e.what(), s, strlen( s)) == 0;
}

/*********************************************************************************
 *	DisplayException - Prints out the string value of an xcode complete with a ¥.
 */
void DisplayException( const std::exception &err)
{
	char	buf[ 1024];
		
	char *start = buf;
	char *end = buf;
	strcpy( buf, err.what());
	
	while( *start)
	{// for each ~ separate portion of the buffer, put on separate line with indent
		end = strchr( start, '~');
		if( end)
		{// This is the next piece
			*end = 0;
			fprintf( stderr, "%s%s,\n", start == buf ? "" : "\t", start);
			start = end+1;
		}
		else
		{// This is the last piece
			fprintf( stderr, "%s%s\n", start == buf ? "" : "\t", start);
			break;
		}
	}
}

void TestFn( int status, const char* msg, const char* file, int line)
{
    if( status)
    {
		if( status == -1)
		{
			status = errno;
			if( msg == NULL) FileLine( file, line) + xraiseit( "Failure status code", "int status", status, "str error", strerror( status), NULL);
			FileLine( file, line) + xraiseit( "Failure status code", "int status", status, "str error", strerror( status), "str action", msg,  NULL);
		}
        if( msg == NULL) FileLine( file, line) + xraiseit( "Failure status code", "int status", status, "str error", strerror(status), NULL);
        FileLine( file, line) + xraiseit( "Failure status code", "int status", status, "str action", msg, "str error", strerror(status), NULL);
    }
}

void TestFn( void* status, const char* msg, const char* file, int line)
{// HA! This is also used for Windows"HANDLE" data type
	if( status == nullptr)
	{// Null return, go after errno;
		TestFn( -1, msg, file, line);
	}
#if WINCODE
	if (status == INVALID_HANDLE_VALUE)
	{// That's no good, treat as error
		TestFn(-1, msg, file, line);
	}
#endif
}

void TestFn(bool status, const char* msg, const char* file, int line)
{
	if( !status)
	{// One must be true to avoid being an exception
		TestFn(-1, msg, file, line);
	}
}

