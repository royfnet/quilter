#pragma once
/*		xraise.h		an xraise package

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
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 4996)
#include <cstring>
#include <exception>
#include <cstdio>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define versionString "3.0.0"	// Not the best place for this

#define CountItems( a) int(sizeof( a)/sizeof( (a)[0]))
#define SUCCESS 1
typedef long long int64;
extern bool noisy;		// Used to key debugging logs

class exexception : public std::exception
{// the kind of exception that is thrown by xraise()
	mutable char whatString[ 2048];
public:
	exexception( const char* s);
	exexception( std::exception* eek);
	void SetFileLine( const char* file, int line) const;
	virtual const char* what() const throw();
};

class FileLine
{// an __FILE__ __LINE__ container, users operator+ to make it work
public:
	const char* iFile;
	const int iLine;
	FileLine( const char* file, const int line) : iFile( file), iLine( line) {}
};

bool frontcmp( const char* pattern, const char* s);
exexception xraiseit( const char* xcode, ...);		// The xraise method
void operator+( const FileLine& f, const exexception& e);

//		Use xraise for unexpected errors, message will include file and line
#define xraise FileLine( __FILE__, __LINE__) + xraiseit
//		Use sraise for user error, message will not include file and line
#define sraise throw xraiseit

bool operator==( const std::exception& e, const char* s);

void DisplayException( const std::exception& err);

#define xassert( p) if( !(p)) xraise( #p, NULL);
#define TestMsg( s, m) TestFn( s, m, __FILE__, __LINE__)
#define Test( s) TestFn( s, #s, __FILE__, __LINE__)
void TestFn( int status, const char* msg, const char* file, int line);
void TestFn( void* status, const char* msg, const char* file, int line);
void TestFn(bool status, const char* msg, const char* file, int line);

#if defined(_WIN32) || defined(_WIN64)
/* We are on Windows */
#define WINCODE 1
#define MACCODE 0

#ifdef DOPLAY			// Set at build time to include play and record commands on Windows
#define PLAYCMD 1
#define USE_ASIO 1
#else
#define PLAYCMD 0
#define USE_ASIO 0
#endif

#define strtok_r strtok_s
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define isatty _isatty

#include <windows.h>
typedef LONG AtomicInttype_t;
typedef LONG64 AtomicInt64type_t;
#define AddInterlocked32( ptr, val) InterlockedAdd( ptr, val)
#define AddInterlocked64( ptr, val) InterlockedAdd64( ptr, val)
typedef int useconds_t;
#define usleep( us) Sleep( (us)/1000)
#define STDIN_FILENO _fileno(stdin)
void TestFn(HANDLE status, const char* msg, const char* file, int line);
#else
/* We are on Mac */

#define WINCODE 0
#define MACCODE 1
#define USE_ASIO 0
#define PLAYCMD 1

typedef int AtomicInttype_t;
typedef int64 AtomicInt64type_t;
#define AddInterlocked32( ptr, val) OSAtomicAdd32Barrier( val, ptr)
#define AddInterlocked64( ptr, val) OSAtomicAdd64Barrier( val, ptr)
#endif
/* end of xraise.h */
