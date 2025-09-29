//
//	ConsoleThings.cpp
//
//	Copyright (c) 2025 Jeffrey Lomicka. All rights reserved.
//
#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>	//Must be before xraise.h that includes windows.h (and defines WINCODE!)
#endif

//#include "xraise.h"	// Included by ConsoleThings.h
#include "ConsoleThings.h"

#if WINCODE
#include "win/getopt.h"
#include "io.h"
#endif

#if MACCODE
#include <unistd.h>
#include <termios.h>
#include <sys/sysctl.h>
#endif
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <math.h>
#include <sys/stat.h>
#include <filesystem>
#include <vector>

bool sTerminalHasCursorBugs = true;

bool AmIBeingDebugged(void)
{// Cache the result so that ATTACHING to a running client doesn't trigger debug behavior
	static bool cachedResult = false;
#if MACCODE
	static bool cachedResultValid = false;
	if( cachedResultValid)
		return cachedResult;
	int mib[4];
	struct kinfo_proc info;
	int size;

	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PID;
	mib[3] = getpid();

	size = sizeof(info);
	info.kp_proc.p_flag = 0;

	sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);

	cachedResult = ((info.kp_proc.p_flag & P_TRACED) != 0);
	cachedResultValid = true;
#endif
	return cachedResult;
}

static char commandline[ 1024];
char* MakeCommandLine( int argc, const char * const argv[])
{
	char* b = commandline;
	int spaceleft = CountItems( commandline)-1;
	commandline[ spaceleft] = 0;	// Ensure terminating nul always
	for( int arg = 0; arg < argc && spaceleft > 0; ++arg)
	{// Copy over as much of the command line as fits
		int len = snprintf( b, spaceleft, "%s ", argv[ arg]);
		b += len;
		spaceleft -= len;
	}
	return commandline;
}

int GetAllOpts(
	int argc, const char * const argv[],
	const char* boolOpts, bool* boolValues[],
	const char* strOpts, const char** strValues[],
	const char* floatOpts, double* floatValues[],
	const char* intOpts, int* intValues[],
	const char* int64Opts, int64* int64Values[],
	const char* paramOpts, const char* const helpMessages[])// Param options are from "SFI.*" for string, float, integer, and uncounted
{
	MakeCommandLine( argc, argv);
	bool displayHelp = false;
	char badchoice[ 2] = {0};
	char optstr[ 64];
	char* o = optstr;
	for( const char* s = boolOpts; s && *s; ++s)
	{
		*o++ = tolower_c( *s);
		*o++ = toupper_c( *s);
		*o++ = ':';
	}
	for( const char* s = strOpts; s && *s; ++s)
	{
		*o++ = *s;
		*o++ = ':';
	}
	for( const char* s = floatOpts; s && *s; ++s)
	{
		*o++ = *s;
		*o++ = ':';
	}
	for( const char* s = intOpts; s && *s; ++s)
	{
		*o++ = *s;
		*o++ = ':';
	}
	for( const char* s = int64Opts; s && *s; ++s)
	{
		*o++ = *s;
		*o++ = ':';
	}

	*o++ = 'h';
	*o = 0;

	opterr = 1;
	optind = 1;
	optreset = 1;

	try
	{
		int optResult = 0;
#if WINCODE
		while( (optResult = getopt(argc, (const char**)(argv), optstr)) >= 0)
#else
		while( (optResult = getopt( argc, const_cast<char * const *>( argv), optstr)) >= 0)
#endif
		{
			if( optResult == '?')
				xraise( "Command line error", nullptr);
			int vindex = 0;		// Value index
			for( const char* s = boolOpts; s && *s; ++s, ++vindex)
			{
				if( optopt == tolower( *s))
				{// Lower case bool option takes no parameter and toggles value
					*boolValues[ vindex] = !*boolValues[ vindex];
					goto break2;
				}
				else if( optopt == toupper( *s))
				{// Uppder case bool option takes parameter 1 or 0
					if( strcmp( optarg, "0") == 0) *boolValues[ vindex] = false;
					else if( strcmp( optarg, "1") == 0) *boolValues[ vindex] = true;
					else xraise( "Boolean options must be 1 or 0", "str found", optarg, NULL);
					goto break2;
				}
			}

			vindex = 0;
			for( const char* s = strOpts; s && *s; ++s, ++vindex)
			{
				if( optopt == *s)
				{
					*strValues[ vindex] = optarg;
					goto break2;
				}
			}

			vindex = 0;
			for( const char* s = floatOpts; s && *s; ++s, ++vindex)
			{
				if( optopt == *s)
				{// ConvertTimeToSeconds includes PitchToFloat and hex conversions for absolute values
					*floatValues[ vindex] = ConvertTimeToSeconds( optarg);
					goto break2;
				}
			}

			vindex = 0;
			for( const char* s = intOpts; s && *s; ++s, ++vindex)
			{
				if( optopt == *s)
				{
					*intValues[ vindex] = xatoi( optarg);
					goto break2;
				}
			}

			vindex = 0;
			for( const char* s = int64Opts; s && *s; ++s, ++vindex)
			{
				if( optopt == *s)
				{
					*int64Values[ vindex] = xatoint64( optarg);
					goto break2;
				}
			}

			if( optopt == 'h')
			{
				displayHelp = true;
				goto break2;
			}

			badchoice[0] = (char) optopt;
			xraise( "Option not recognized.	 Use -h to list available options.", "str option", badchoice, NULL);

			break2:;	// Success, get next option
		}

		if( paramOpts && !displayHelp)
		{// Parameter checking, but skip if display help
			int requiredParams = strli( paramOpts);
			if( strchr( paramOpts, '.'))
			{// There are a minimum number of parameters, but could be much more
				if( argc - optind < requiredParams-1)
				{
					printf( "The %s command requires at least %d parameters.\n", argv[ 0], requiredParams-1);
					displayHelp = true;
				}
			}
			else if( strchr( paramOpts, '*'))
			{// There are a minimum number of parameters, the set of parameters are completely optional
				if( argc - optind < requiredParams-2)
				{
					printf( "The %s command requires at least %d parameters.\n", argv[ 0], requiredParams-2);
					displayHelp = true;
				}
			}
			else
			{
			   if( argc - optind != requiredParams)
				{
					printf( "The %s command requires %d parameters.\n", argv[ 0], requiredParams);
					displayHelp = true;
				}
			}

			// Validate parameter types

			char typeCode = 'S';
			int localOptInd = optind;
			for( int a=0; a < requiredParams && localOptInd + a < argc; a++)
			{// No need to check type code S
				typeCode = paramOpts[ a];
				if( paramOpts[ a+1] == '*')
				{// Handle the case where this paraemter is optional by going diretly to the ... case
					typeCode = '.';
				}
				if( typeCode == 'F') PitchToFreq( argv[ localOptInd + a]);
				else if( typeCode == 'I') xatoi( argv[ localOptInd + a]);
				else if( typeCode == 'L') xatoint64( argv[ localOptInd + a]);
				else if( typeCode == '.')
				{// Check the rest according to the last type code
					while( a < argc)
					{// Check all of the remaining parameters
						if( typeCode == 'F') PitchToFreq( argv[ localOptInd + a]);
						else if( typeCode == 'I') xatoi( argv[ localOptInd + a]);
						else if( typeCode == 'L') xatoint64( argv[ localOptInd + a]);
						++a;
					}
					break;
				}
			}
		}
	}
	catch( std::exception& err)
	{// This is for catchng malformed parameter values
		DisplayException( err);
		displayHelp = true;
	}

	if( displayHelp)
	{
		int oindex = 0;
		int vindex = 0;

		const char * thisCommand = strrchr( argv[ 0], '/');
		if( thisCommand == nullptr)
			thisCommand = argv[ 0];
		else
			++thisCommand;
		while( *thisCommand == ' ')
			++thisCommand;
		printf( "Options for %s are:\n", thisCommand);

		for( const char* s = boolOpts; s && *s; ++s)
		{
			printf( "-%c, -%c0, -%c1, default: %s, %s\n",
				tolower( *s), toupper( *s), toupper( *s), *boolValues[ vindex++] ? "on" : "off",
				helpMessages ? helpMessages[ oindex++]: "Toggles, or turns option on or off.");
		}
		vindex = 0;
		for( const char* s = strOpts; s && *s; ++s)
		{
			printf( "-%c <text>, default: %s, %s\n",
				*s, *strValues[ vindex] ? *strValues[ vindex] : "(empty)",
				helpMessages ? helpMessages[ oindex++]: "Text value.");
			vindex++;
		}
		vindex = 0;
		for( const char* s = floatOpts; s && *s; ++s)
		{
			printf( "-%c <float>, default %.2f, %s\n",
				*s, *floatValues[ vindex++],
				helpMessages ? helpMessages[ oindex++]: "Floating point number.");
		}
		vindex = 0;
		for( const char* s = intOpts; s && *s; ++s)
		{
			printf( "-%c <integer>, default %d, %s\n",
				*s, *intValues[ vindex++],
				helpMessages ? helpMessages[ oindex++]: "Number, integer.");
		}
		vindex = 0;
		for( const char* s = int64Opts; s && *s; ++s)
		{
			printf( "-%c <integer>, default %lld, %s\n",
				*s, *int64Values[ vindex++],
				helpMessages ? helpMessages[ oindex++]: "Number, integer, could be very large.");
		}
		vindex = 0;
		for( const char* s = paramOpts; s && *s; ++s, vindex++)
		{
			if( *s == '.')
			{
				printf( "Add as many as needed.\n");
				break;
			}
		   if( s[1] == '*')
			{
				printf( "Parameter %d, %s\n", vindex+1, helpMessages ? helpMessages[ oindex++]: "Optional, as many as needed.\n");
				break;
			}
			printf( "Parameter %d, %s\n", vindex+1, helpMessages ? helpMessages[ oindex++]: "Required.");
		}
/*
		printf( "The option -h will display this help.\n");
		if( floatOpts)
		{
			printf( "Float values can be a number, in decimal, or in hexadecimal, if prefixed by $ or 0x (don't use $ from shell!),\n");
			printf( "or a musical note from A0 to G#8, to specify a frequency on a equal tempered scale with A4=440.\n");
		}
*/
//		printf( "Version %s %s %s\n", versionString, __DATE__, __TIME__);
		sraise( "Stopped by -h.", nullptr);
	}
	return optind;
}


ScopedGetch::ScopedGetch()
:
	iTypeAheadLength( 0)
{
	memset( &iTypeAhead, 0, sizeof( iTypeAhead));
#if MACCODE
	memset( &noncon, 0, sizeof( noncon));
	TestMsg( tcgetattr( STDIN_FILENO, &noncon), "tcgetattr");
	noncon.c_lflag &= ~ICANON;
	noncon.c_lflag &= ~ECHO;
	TestMsg( tcsetattr( STDIN_FILENO, TCSANOW, &noncon), "tcsetattr");
#endif

}

ScopedGetch::~ScopedGetch()
{
#ifndef _WIN32
	noncon.c_lflag |= ICANON;
	noncon.c_lflag |= ECHO;
	TestMsg( tcsetattr( STDIN_FILENO, TCSANOW, &noncon), "tcsetattr");
#endif
}

#if MACCODE
size_t ScopedGetch::ReadBuf( char* buf, int len)
{
	fd_set rfds;
	struct timeval tv;

	FD_ZERO( &rfds);
	FD_SET( STDIN_FILENO, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = 500000;
	int notTimeOut = select( STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);
	if( notTimeOut == 0) return 0;
	else if (notTimeOut > 0)
	{
		return read(STDIN_FILENO, buf, len);

	}
	else TestMsg( errno, strerror( errno));
	return 0;
}

int ScopedGetch::ReadChar()
{
	for(;;)
	{// Until we get something or time out
		if( iTypeAheadLength > 0)
		{// Something buffered up, return it
			int retval = *iTypeAhead;
			memcpy( iTypeAhead, iTypeAhead+1, --iTypeAheadLength);
			return retval;
		}
		iTypeAheadLength = ReadBuf( iTypeAhead, CountItems( iTypeAhead));
		if( iTypeAheadLength == 0)
			return 0;
	}
}
#endif
#if WINCODE
#include "win/converter.h"
 int ScopedGetch::ReadChar()
{
   if (iTypeAheadLength > 0)
	{// Something buffered up (happens when app calls GetLine() with nullptr), return it
		char retval = *iTypeAhead;
		memcpy(iTypeAhead, iTypeAhead + 1, --iTypeAheadLength);
		return retval;
	}
	HANDLE eventHandles[] =
	{
		GetStdHandle(STD_INPUT_HANDLE)
	// ... add more handles and/or sockets here
	};
	Test(eventHandles[0]);
	DWORD result = WSAWaitForMultipleEvents(
		(DWORD) (CountItems(eventHandles)),
		&eventHandles[0],
		(BOOL) FALSE,
		(DWORD) 500,
		(BOOL) TRUE
	);

	INPUT_RECORD record;
	DWORD numRead = 0;

	if( result == WSA_WAIT_EVENT_0)
	{
		Test( (bool) ReadConsoleInput( eventHandles[0], &record, 1, &numRead));
		if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown)
		{ // key up event, return the decoded character
			if (record.Event.KeyEvent.uChar.UnicodeChar)
			{
				iTypeAheadLength = utf16_to_utf8((utf16_t*) & record.Event.KeyEvent.uChar.UnicodeChar, 1, (utf8_t*)iTypeAhead, CountItems(iTypeAhead));
				return ReadChar();
			}
			if(record.Event.KeyEvent.uChar.AsciiChar)
				return record.Event.KeyEvent.uChar.AsciiChar;
			switch (record.Event.KeyEvent.wVirtualKeyCode)
			{
			case VK_UP: return kUpArrow;
			case VK_DOWN: return kDownArrow;
			case VK_LEFT: return kLeftArrow;
			case VK_RIGHT: return kRightArrow;
			case VK_DELETE: return kDelFwd;
			default: break;
			}
			return 0;		// Other keys do nothing right now
		}
	}
	return 0;
}
#endif

int ScopedGetch::ReadCmd()
{// Reads some escape sequenes into named keys
	int paramVal = 0;	// Escape sequence parameter value from ReadCmd():
	int retval = ReadChar();
	if( retval == 27)
	{
		retval = ReadChar();
		if( retval == '[')
		{
			for(;;)
			{
				retval = ReadChar();
				if( !isdigit( retval))
					break;
				paramVal = paramVal * 10 + (retval - '0');
			}
			if( retval != 0)
			{// High order byte is parameter value, rest is the actual escape sequence
				retval = (paramVal << 24) | 0x1b5b00 | retval;
			}
		}
	}
	return retval;
}

void ScopedGetch::ReadLine( char* buf, int buflen)
{
//		Leave single character mode
#ifndef _WIN32
	noncon.c_lflag |= ICANON;
	noncon.c_lflag |= ECHO;
	TestMsg( tcsetattr( STDIN_FILENO, TCSANOW, &noncon), "tcsetattr");
#endif
	char* result;
	if( buf && buflen > 0)
	{
		Test( result = fgets( buf, buflen-1, stdin));
	}
	else
	{
		Test( result = fgets( &iTypeAhead[ iTypeAheadLength], 64-(int)iTypeAheadLength, stdin));
		if( result)
			iTypeAheadLength += strli( result);
	}

//		Return to single character mode
#ifndef _WIN32
	noncon.c_lflag &= ~ICANON;
	noncon.c_lflag &= ~ECHO;
	TestMsg( tcsetattr( STDIN_FILENO, TCSANOW, &noncon), "tcsetattr");
#endif
}


int xatoi( const char* s)
{
	char* done;
	errno = 0;
	int64 checkval = strtoll( s, &done, 10);
	if( (done != s + strlen( s)) || (checkval > INT_MAX) || (checkval < INT_MIN) || (errno != 0))
		xraise( "Value is not an integer", "str value", s, NULL);
	return (int)checkval;
}

int64 xatoint64( const char* s)
{
	char* done;
	errno = 0;
	int64 checkval = strtoll( s, &done, 10);

	if( (done != s + strlen( s)) || (errno != 0))
		xraise( "Value is not an integer", "str value", s, NULL);
	return checkval;
}

double xatof( const char* s)
{
	char* done;
	errno = 0;
	double checkval = strtod( s, &done);
		if( (done != s + strlen( s)) || (errno != 0))
			xraise( "Value is not a number", "str value", s, NULL);
	return checkval;
}

double ConvertAsciiToFloat( const char* curArg)
{// Like xatof except that it also allows hexadecimal.	Primarily for amplitudes.  I might make it accept notes too, for freqencies
	if( (curArg[0] == '.') || (curArg[0] == '-') || (isdigit( curArg[0]) && curArg[1] != 'x') )
		return atof( curArg );
	// First check to see if it's definitely a number.
	if( ((curArg[0] == '$') && (curArg[1] != 0))		// $ means hex
	   ||
	   ((curArg[0] == '0') && (curArg[1] == 'x') && (curArg[2] != 0))	// 0x means hex
	   ||
	   ((curArg[0] == '0') && (curArg[1] == 'X') && (curArg[2] != 0)))	// 0X means hex
	{// This ought to be a hexadecimal number of at least one digit
		int retVal = 0;
		const char *digit = curArg + 1;
		if( *digit == 'x' || *digit == 'X') digit++;
		static const char *hexDigits = "0123456789abcdef";	// Tokenizer has lower cased the number
		for(;;)
		{// Accumulate hex digits
			const char *valueIndex = strchr( hexDigits, *digit);
			if( valueIndex == 0) break;		// Wasn't a hex digit, look up as symbol
			retVal = (retVal << 4) + (int)(valueIndex - hexDigits);	// Pick up digit's value
				digit++;
			if( *digit == 0)
			{// Got an entire number, return the value
				return retVal;
			}
		}
	}
	if ((isdigit(curArg[0])) || (curArg[0] == '-'))
	{
		return xatof( curArg);
	}
	xraise( "Not a number", "str curarg", curArg, NULL);
	return 0.0;
}

double ConvertTimeToSeconds( const char* curArg)
{// Converts dd::hh:mm:ss.sss to seconds
	char tempstr[ 256];

	const char* colon = strrchr( curArg, ':');
	if( colon == nullptr)
		return PitchToFreq( curArg);

// Pull off the seconds field, everything after the last ":"
	double seconds = ConvertAsciiToFloat( colon + 1);

// Now look for minutes
	if( colon == curArg)
		return seconds; // There are no minutes
// Find the start of the minutes string
	memset( tempstr, 0, sizeof( tempstr));
	strncpy( tempstr, curArg, colon - curArg);
	char* colonm = strrchr( tempstr, ':');
	if( colonm == nullptr)
		colonm = tempstr;
	else
		++colonm;
	double minutes = (double) xatoi( colonm);
	seconds += minutes * 60;

// Now look for hours
	if( colonm == tempstr)
		return seconds; // There are no hours
// Find the start of the hours string
	colonm[ -1] = 0;
	char* colonh = strrchr( tempstr, ':');
	if( colonh == nullptr)
		colonh = tempstr;
	else
		++colonh;
	double hours = (double) xatoi( colonh);
	seconds += hours * 3600;

// Now look for days
	if( colonh == tempstr)
		return seconds; // There are no days
	colonh[ -1] = 0;
	double days = (double) xatoi( tempstr);
	seconds += days * 3600 * 24 + seconds;

	return seconds;
}

#if WINCODE
#include <iomanip>
#include <sstream>

extern "C" const char* strptime(
	const char* s,
	const char* f,
	struct tm* tm)
{
	// Isn't the C++ standard lib nice? std::get_time is defined such that its
	// format parameters are the exact same as strptime. Of course, we have to
	// create a string stream first, and imbue it with the current C locale, and
	// we also have to make sure we return the right things if it fails, or
	// if it succeeds, but this is still far simpler an implementation than any
	// of the versions in any of the C standard libraries.
	std::istringstream input(s);
	input.imbue(std::locale(setlocale(LC_ALL, nullptr)));
	input >> std::get_time(tm, f);
	//if (input.eof()) return (char*)(s + strlen(s));
	if (input.fail()) {
		return nullptr;
	}
	return s + static_cast<std::streamoff>(input.tellg());
}
#endif

// function to parse a date or time string.
time_t xatot(const char* datetimeString)
{
	char scrap[ 256] = {0};
//		Get default values for date and time, in case they are missing
	time_t rawtime;
	time (&rawtime);
	struct tm now;
#if WINCODE
	now = *(localtime(&rawtime));
#else
	localtime_r( &rawtime, &now);
#endif
	//snprintf( scrap, 254, "%04d-%02d-%02d %02d:%02d:%02d", now.tm_year+1900,now.tm_mon+1,now.tm_mday,now.tm_hour,now.tm_min,now.tm_sec);

	if( !strchr( datetimeString, '-'))
		snprintf( scrap, 254, "%04d-%02d-%02d %s", now.tm_year+1900, now.tm_mon+1, now.tm_mday, datetimeString);
	else if( !strchr( datetimeString, ':'))
		snprintf( scrap, 254, "%s %02d:%02d:%02d", datetimeString, now.tm_hour, now.tm_min,now.tm_sec);
	else
		strncpy( scrap, datetimeString, 254);
	struct tm tmStruct;
	if( strptime(scrap, "%Y-%m-%d %H:%M:%S", &tmStruct) < &scrap[ 10])
		xraise( "Date/Time should be of form YY-MM-DD hh:mm:ss", "str received", datetimeString, nullptr);
	rawtime = mktime( &tmStruct);
	return rawtime;
}


class note_t
{
public:
	double iFreq;
	char iName[ 4];
	char* sname( int* len)
	{
		if( len) *len = strli( iName);
		return iName;
	}
	
	char* dname( int* len, int)
	{
		if( len) *len = strli( iName);
		return iName;
	}
	
	note_t( char* name, double freq)
	{
		strcpy( iName, name);
		iFreq = freq;
	}
};

#include "lut.h"
static lut<note_t> sNoteList;

double PitchToFreq( const char* arg)
{// For accepting pitch on the command line as substitute for frequency
	// a4 is 440hz
	if( sNoteList.count( NULL, 0, NULL) == 0)
	{// No list yet, make it
		const char * notess[] = { "c", "c#", "d", "d#", "e", "f", "f#", "g", "g#", "a", "a#", "b"};
		const char * notesf[] = { "c", "db", "d", "eb", "fb", "f", "gb", "g", "ab", "a", "bb", "cb"};
		// Also the upper case versions
		const char * notesS[] = { "C", "C#", "D", "D#", "E", "F", "F#", "`G", "G#", "A", "A#", "B"};
		const char * notesF[] = { "C", "Db", "D", "Eb", "Fb", "F", "Gb", "G", "Ab", "A", "Bb", "Cb"};
		const char ** allNotes[] = {notess, notesf, notesS, notesF};
		double a4 = 440.0;
		double interval = pow( 2.0, 1.0/12.0);
		for( int scale = 0; scale < CountItems( allNotes); ++scale)
		{
			double myfreq = pow( 2.0, -9.0/12.0) * (a4 / 16.0);		// C0, first note
			for( char octave = 0; octave < 9; octave++)
			{
				for( int n = 0; n < CountItems( notess); n++)
				{
					char name[ 4];
					char o[ 2];
					o[ 1] = 0;
					strcpy( name, allNotes[ scale][ n]);
					o[ 0] = octave + '0';
					strcat( name, o);
					if( !sNoteList.lookup( name, strli( name)))
					{// This is not a duplicate, we we can add it to the list.
					  // Skips duplicates that exist in both the sharp list and the flat list.
						note_t* nn = new note_t( name, myfreq);
						sNoteList.add( nn);
					}
					myfreq = interval * myfreq;
				}
			}
		}
	}
	note_t* thisNote = sNoteList.lookup( arg, strli( arg));
	if( thisNote)
		return thisNote->iFreq;
	return ConvertAsciiToFloat( arg);
}

void PrintAllNotes()
{
	PitchToFreq( "a4"); // Initialize table, if needed
	int count;
	note_t* nn = sNoteList.lookup_a( NULL, 0, count);
	for(; count--; nn = sNoteList.next( nn))
	{
		printf("%4.4s %f\n",
			   nn->iName,
			   nn->iFreq);
	}
}

int64 CommaSeparatedListOfBits( const char* bitStr)
{// Converts a comma separated list of numbers into a 64-bit bit mask.
	if( bitStr == nullptr)
		return 0;
	char num[3];
	int64 retval = 0;
	while( *bitStr)
	{
		const char* comma = strchr( bitStr, ',');
		intptr_t len = comma ? comma-bitStr : strli( bitStr);
		if( len > CountItems( num)-1)
			xraise( "Bad number", "str given", bitStr, nullptr);
		strncpy( num, bitStr, len);
		num[ len] = 0;
		int bit = xatoi( num);
		if( bit < 1 ||	bit > 64)
			xraise( "Number out of range", "int given", bit, nullptr);
		retval |= (int64)1<<(bit-1);
		bitStr += len;
		if( *bitStr) ++bitStr;	// Skip over the comma
	}
	return retval;
}

const char** CommaSeparatedListOfValues( const char* valueStr, int* argcptr)
{// Converts a comma separated list into argc/argv, quotes not handled yet (maybe steal code from TinyXML?)
 // Return values are all new allocations. We probalby leak them.
	const char** retval = new const char*[ 100];	// Max out at 100
	*argcptr = 0;
	if( valueStr == nullptr)
		return retval;

	while( *valueStr && *argcptr < 100)
	{
		const char* comma = strchr( valueStr, ',');
		intptr_t len = comma ? comma-valueStr : strli( valueStr);
		char* num = new char[len+1];
		retval[ (*argcptr)++] = num;
		strncpy( num, valueStr, len);
		num[ len] = 0;
		valueStr += len;
		if( *valueStr) ++valueStr;	// Skip over the comma
	}
	return retval;
}

char* FilenameWithChannel( char* nameBuffer, const char* baseFileName, const char* suffix, int channel, const char* type)
{
//		Find a type (extension) to use
	const char* fileType = strrchr( baseFileName, '.');
	if( fileType == NULL)
	{// File type not given in the base file name, use the provided one instead (if there is one)
		fileType = type ? type : "";
	}
	else
	{// File type given, point to that one
		fileType++;
	}
	if( type && channel < 0)
	{// Set channel to -1 to OVERRIDE the given file type with the one provieded (ie, .xml)
		fileType = type;
	}

	strcpy( nameBuffer, baseFileName);
	char* nb = strrchr( nameBuffer, '.');
	if( nb == nullptr)
		nb = nameBuffer + strlen( nameBuffer);
	int rem = 1024 - int( nb-nameBuffer);
	if( channel > 0)
		snprintf( nb, rem, "-%s%02d.%s", suffix, channel, fileType);
	else
		snprintf( nb, rem, "-%s.%s", suffix, fileType);

	return nameBuffer;
}

bool DefaultFileName( char* givenFileName, char* fileNameBuffer, int bufSize, const char* type)
{// Returns TRUE if the provided givenFileName exists and is not a folder
//		givenFileName may be null.
//		The given file name may be an entire file name or just a folder, and if it came
//		off the filename itself on Mac, it may have had "/" replaced with ":", so flip
//		them back.

	struct stat st = {0};
	if( type == nullptr)
		type = ".wav";		// We default to .wav, 'cause record doesn't support other formats
	if( *type == '.')
		++type;				// remove the "." if given

	char* givenFolderName = nullptr;
	if( givenFileName)
	{// Filename given on command line
#if MACCODE
		for( char*s = givenFileName; *s; ++s)
		{
			if( *s == ':')	// Replace all ":" with "/" to undo damage done by MacOS
				*s = '/';
			if( *s == '/' && s[1] == 0)
				*s = 0;		// Removing terminating "/" if any
		}
#endif
#if WINCODE
		for (char* s = givenFileName; *s; ++s)
		{// Convert Windows path to Unix path?
			if (*s == '/' && s[1] == 0)
				*s = 0;		// Removing terminating "/" if any
			//if (*s == '\\')
			//	  *s = '/';
		}
#endif
//			Determine if this file exists as a folder, and if so, use it as the folder
//			but continue with the default name.
		if( stat( givenFileName, &st) == 0)
		{// It exists, is it a folder?
			if( (st.st_mode & S_IFDIR) != 0)
			{// It is a folder, use it as the folder name
				givenFolderName = givenFileName;
			}
		}
		if( givenFolderName == nullptr)
		{// Full file name was given, use it
			strncpy( fileNameBuffer, givenFileName, bufSize);
			if( strrchr( fileNameBuffer, '.') == nullptr)
			{// If no type given in the file name, append it
				strcat( fileNameBuffer, ".");
				strcat( fileNameBuffer, type);
			}
			return true;
		}
	}

	time_t rawtime;
	time (&rawtime);
	struct tm now;
#if WINCODE
	now = *(localtime(&rawtime));
#else
	localtime_r( &rawtime, &now);
#endif
	if( givenFolderName == nullptr)
	{// Didn't get a folder name from command line, make one now
		snprintf( fileNameBuffer, 1024, "%04d", now.tm_year+1900);
		if( stat( fileNameBuffer, &st) != 0)
		{// Folder doesn't exist, create it with the parent folder's mode
			Test( stat( ".", &st));
#if WINCODE
			Test(std::filesystem::create_directory( fileNameBuffer));
#endif
#if MACCODE
			Test( mkdir( fileNameBuffer, st.st_mode & 0x1FF));	 // Mignt need to revert to this on Mac.
#endif
			Test( stat( fileNameBuffer, &st));	// Should work now
		}
		if( (st.st_mode & S_IFDIR) == 0)
		{// Not a directory!
			xraise( "Destination folder is not a folder", "str folder", fileNameBuffer, NULL);
		}
		givenFolderName = fileNameBuffer;
	}
	snprintf( fileNameBuffer, 1024, "%s/%04d-%02d-%02d-%02d%02d%02d.%s", givenFolderName, now.tm_year+1900,now.tm_mon+1,now.tm_mday,now.tm_hour,now.tm_min,now.tm_sec,type);
	return false;
}

static const int kMaxArgs = 64;		// The argv vector after ripping aprt argv[0] into individual parameters by ScrapeArgs
static char *myArgv[ kMaxArgs];

void ScrapeArgs( int* argcp, char ***argvp)
{
	int argc = *argcp;
	char **argv = *argvp;

	if( argc == 1)
	{// If the only parameter is the executable name, turn it into the command line
		argc = 0;

		char* nextParam = strrchr( argv[ 0], '/');	// Strip off the path at the beginning of the command, because it might have spaces in it
		if( nextParam == nullptr) nextParam = argv[ 0]; // I know this means I can't have '/' in any of the parameters.
		else ++nextParam;

		for(;;)
		{// Accumulate parameters based on spaces, for this application we don't worry about quoted parameters
			myArgv[ argc++] = nextParam;
			nextParam = strchr( nextParam, ' ');
			if( nextParam == nullptr) break;
			while( *nextParam == ' ') *nextParam++ = 0;
			if( argc >= kMaxArgs) break;
		}
		argv = myArgv;
		// for( int i = 0; i < argc; i++) printf( "%d: %s\n", i, argv[ i]);
		*argcp = argc;
		*argvp = myArgv;
	}
}

// Variations
int GetAllOpts(
	int argc, const char * const argv[],
	const char* strOpts, const char** strValues[])
{
	return GetAllOpts(
		argc, argv,
		nullptr, nullptr,
		strOpts, strValues,
		nullptr, nullptr,
		nullptr, nullptr,
		nullptr, nullptr,
		nullptr, nullptr);
}

int GetAllOpts(
	int argc, const char * const argv[],
	const char* boolOpts, bool* boolValues[],
	const char* strOpts, const char** strValues[])
{
	return GetAllOpts(
		argc, argv,
		boolOpts, boolValues,
		strOpts, strValues,
		nullptr, nullptr,
		nullptr, nullptr,
		nullptr, nullptr,
		nullptr, nullptr);
}


int GetAllOpts(
	int argc, const char * const argv[],
	const char* boolOpts, bool* boolValues[],
	const char* strOpts, const char** strValues[],
	const char* floatOpts, double* floatValues[])
{
	return GetAllOpts(
		argc, argv,
		boolOpts, boolValues,
		strOpts, strValues,
		floatOpts, floatValues,
		nullptr, nullptr,
		nullptr, nullptr,
		nullptr, nullptr);
}

int GetAllOpts(
	int argc, const char * const argv[],
	const char* floatOpts, double* floatValues[],
	const char* intOpts, int* intValues[])
{
	return GetAllOpts(
		argc, argv,
		nullptr, nullptr,
		nullptr, nullptr,
		floatOpts, floatValues,
		intOpts, intValues,
		nullptr, nullptr,
		nullptr, nullptr);
}

int GetAllOpts(
	int argc, const char * const argv[],
	const char* boolOpts, bool* boolValues[],
	const char* strOpts, const char** strValues[],
	const char* floatOpts, double* floatValues[],
	const char* intOpts, int* intValues[])
{
	return GetAllOpts(
		argc, argv,
		boolOpts, boolValues,
		strOpts, strValues,
		floatOpts, floatValues,
		intOpts, intValues,
		nullptr, nullptr,
		nullptr, nullptr);
}

int GetAllOpts(
	int argc, const char * const argv[],
	const char* boolOpts, bool* boolValues[],
	const char* strOpts, const char** strValues[],
	const char* floatOpts, double* floatValues[],
	const char* intOpts, int* intValues[],
	const char* int64Opts, int64* int64Values[])
{
	return GetAllOpts(
		argc, argv,
		boolOpts, boolValues,
		strOpts, strValues,
		floatOpts, floatValues,
		intOpts, intValues,
		int64Opts, int64Values,
		nullptr, nullptr);
}


int GetAllOpts(
	int argc, const char * const argv[],
	const char* strOpts, const char** strValues[],
	const char* floatOpts, double* floatValues[],
	const char* intOpts, int* intValues[])
{
	return GetAllOpts(
		argc, argv,
		nullptr, nullptr,
		strOpts, strValues,
		floatOpts, floatValues,
		intOpts, intValues,
		nullptr, nullptr,
		nullptr, nullptr);
}

// Variations with params and help

int GetAllOpts(
	int argc, const char * const argv[],
	const char* strOpts, const char** strValues[],
	const char* paramOpts, const char* const helpMessages[])
{
	return GetAllOpts(
		argc, argv,
		nullptr, nullptr,
		strOpts, strValues,
		nullptr, nullptr,
		nullptr, nullptr,
		nullptr, nullptr,
		paramOpts, helpMessages);
}

int GetAllOpts(
	int argc, const char * const argv[],
	const char* boolOpts, bool* boolValues[],
	const char* strOpts, const char** strValues[],
	const char* paramOpts,const char* const helpMessages[])
{
	return GetAllOpts(
		argc, argv,
		boolOpts, boolValues,
		strOpts, strValues,
		nullptr, nullptr,
		nullptr, nullptr,
		nullptr, nullptr,
		paramOpts, helpMessages);
}

int GetAllOpts(
	int argc, const char * const argv[],
	const char* strOpts, const char** strValues[],
	const char* floatOpts, double* floatValues[],
	const char* intOpts, int* intValues[],
	const char* paramOpts, const char* const helpMessages[])
{
	return GetAllOpts(
		argc, argv,
		nullptr, nullptr,
		strOpts, strValues,
		floatOpts, floatValues,
		intOpts, intValues,
		nullptr, nullptr,
		paramOpts, helpMessages);
}

int GetAllOpts(
	int argc, const char * const argv[],
	const char* strOpts, const char** strValues[],
	const char* floatOpts, double* floatValues[],
	const char* paramOpts, const char* const helpMessages[])
{
	return GetAllOpts(
		argc, argv,
		nullptr, nullptr,
		strOpts, strValues,
		floatOpts, floatValues,
		nullptr, nullptr,
		nullptr, nullptr,
		paramOpts, helpMessages);
}

int GetAllOpts(
	int argc, const char * const argv[],
	const char* boolOpts, bool* boolValues[],
	const char* strOpts, const char** strValues[],
	const char* floatOpts, double* floatValues[],
	const char* paramOpts, const char* const helpMessages[])
{
	return GetAllOpts(
		argc, argv,
		boolOpts, boolValues,
		strOpts, strValues,
		floatOpts, floatValues,
		nullptr, nullptr,
		nullptr, nullptr,
		paramOpts, helpMessages);
}

int GetAllOpts(
	int argc, const char * const argv[],
	const char* boolOpts, bool* boolValues[],
	const char* strOpts, const char** strValues[],
	const char* intOpts, int* intValues[],
	const char* paramOpts, const char* const helpMessages[])
{
	return GetAllOpts(
		argc, argv,
		boolOpts, boolValues,
		strOpts, strValues,
		nullptr, nullptr,
		intOpts, intValues,
		nullptr, nullptr,
		paramOpts, helpMessages);
}

int GetAllOpts(
	int argc, const char * const argv[],
	const char* floatOpts, double* floatValues[],
	const char* intOpts, int* intValues[],
	const char* paramOpts, const char* const helpMessages[])
{
	return GetAllOpts(
		argc, argv,
		nullptr, nullptr,
		nullptr, nullptr,
		floatOpts, floatValues,
		intOpts, intValues,
		nullptr, nullptr,
		paramOpts, helpMessages);
}

int GetAllOpts(
	int argc, const char * const argv[],
	const char* boolOpts, bool* boolValues[],
	const char* strOpts, const char** strValues[],
	const char* floatOpts, double* floatValues[],
	const char* intOpts, int* intValues[],
	const char* paramOpts, const char* const helpMessages[])
{
	return GetAllOpts(
		argc, argv,
		boolOpts, boolValues,
		strOpts, strValues,
		floatOpts, floatValues,
		intOpts, intValues,
		nullptr, nullptr,
		paramOpts, helpMessages);
}

int Dispatch(		// Disptach on keyword
	CommandProc* cur,
	const char** cmds,
	int (** rtns)( CommandProc* cur))
{
	int c;
	const char* cmd = "";

	if( cur->iArgc > 0)
	{// There is a keyword, dispatch on it
		cmd = cur->iArgv[ 0];
		for( c = 0; cmds[ c] != 0; c++)
		{// For each keyword in the table, check for it
			if( strcmp( cmd, cmds[ c]) == 0)
			{// Found it, run the command
				return (rtns[ c])( cur);
			}
		}
	}

	printf( "Expected one of:\n");
	for( c = 0; cmds[ c] != 0; c++)
	{// For each keyword, display it
		printf( "	 %s\n", cmds[ c]);
	}
	xraise( "Keyword not found", "str keyword", cmd, NULL);
	return 1;
}

const char* helpflag = "-h";

void DoHelp(
	const char* defaultCommand, // Should never need to be used
	CommandProc* cur,
	const char** cmds,
	int (** rtns)( CommandProc* cur))
{// Iterates over the command name requesting help on all of them.
	int c;
	const char* args[ 3];
	CommandProc cmd( 2, args);
	char commandArg[ 128];
	const char *commandName = defaultCommand;  // Default, but should never need to be used

	if( cur->iArgc > 0)
	{// Comand given (it shoudl always be), Skip over the path name from front of the command name, if needed
		commandName = strrchr( cur->iArgv[ 0], '/');
		if( commandName) commandName++;
		else commandName = cur->iArgv[ 0];
	}

	for( c = 0; cmds[ c] != 0; c++)
	{// For each keyword in the table, run a -h on it
		snprintf( commandArg, CountItems( commandArg), "%s %s", commandName, cmds[ c]); // Build command line
		args[ 0] = commandArg;
		args[ 1] = helpflag;
		args[ 2] = nullptr;
 //		  printf( "\n%s command\n", commandArg);
		printf( "\n");
		try { (rtns[ c])( &cmd);} catch (...) {}	// Eat the exit exception
	}
}

static int AppendOpts(
	char* destOptions, void* destValues,
	const char* sourceOptions, void* sourceValues)
{
	if( sourceOptions == nullptr)
		return 0;
	int oldLength = strli( destOptions);
	int addLength = strli( sourceOptions);
	strcpy( &destOptions[ oldLength], sourceOptions);
	void** destValuesReally = (void**) destValues;
	memcpy( &destValuesReally[ oldLength], sourceValues, addLength * sizeof( void*));
	return addLength;
}

int GetAllOpts(		// Concatenates options
	int argc, const char * const argv[],
	AllOpts_t **opts,
	int count)		// If 0, we're looking for a nulptr termianted list
{
	if( count == 0)
	{// Need to count them
		for( AllOpts_t** thisOpt = opts; *thisOpt++ != nullptr; ++count);
	}
	if( count == 1)
	{// Optimization for simple case
		return GetAllOpts( argc, argv,
			opts[ 0]->boolOpts, opts[ 0]->boolValues,
			opts[ 0]->strOpts, opts[ 0]->strValues,
			opts[ 0]->floatOpts, opts[ 0]->floatValues,
			opts[ 0]->intOpts, opts[ 0]->intValues,
			opts[ 0]->int64Opts, opts[ 0]->int64Values,
			opts[ 0]->paramOpts, opts[ 0]->helpMessage);
	}
	else
	{// We'll keep this simple by preallocating reasonable maximums for combined options
		const int kMaxOpts = 32;
		char combinedBoolOpts[ kMaxOpts] = { 0};
		bool* combinedBoolValues[ kMaxOpts];
		char combinedStrOpts[ kMaxOpts] = { 0};
		const char** combinedStrValue[ kMaxOpts];
		char combinedFloatOpts[ kMaxOpts] = { 0};
		double* combinedFloatValue[ kMaxOpts];
		char combinedIntOpts[ kMaxOpts] = { 0};
		int* combinedIntValues[ kMaxOpts];
		char combinedInt64Opts[ kMaxOpts] = { 0};
		int64* combinedInt64Value[ kMaxOpts];
		char combinedParamOpts[ kMaxOpts] = { 0};
		char* combinedHelpMessage[ kMaxOpts*7];

		int numOfCombinedHelps = 0;
		int posOfNextHelp[ kMaxOpts] = {0};		// assert count < kMaxOpts
		for( int opt = 0; opt < count; ++opt)
		{// Build the combined parameter list.	Doing this one at a time because the helps have to be in order
		 // of type, that is, all bool, all string, all float, all in, all int64, then params
			int numOfSrcHelps = AppendOpts( combinedBoolOpts, combinedBoolValues, opts[ opt]->boolOpts, opts[ opt]->boolValues);
			memcpy( &combinedHelpMessage[ numOfCombinedHelps], opts[ opt]->helpMessage, numOfSrcHelps *sizeof( char*));
			posOfNextHelp[ opt] = numOfSrcHelps;
			numOfCombinedHelps = numOfSrcHelps;
		}
		for( int opt = 0; opt < count; ++opt)
		{
			int numOfSrcHelps = AppendOpts( combinedStrOpts, combinedStrValue, opts[ opt]->strOpts, opts[ opt]->strValues);
			memcpy( &combinedHelpMessage[ numOfCombinedHelps], &opts[ opt]->helpMessage[ posOfNextHelp[ opt]], numOfSrcHelps *sizeof( char*));
			posOfNextHelp[ opt] += numOfSrcHelps;
			numOfCombinedHelps += numOfSrcHelps;
		}
		for( int opt = 0; opt < count; ++opt)
		{
			int numOfSrcHelps = AppendOpts( combinedFloatOpts, combinedFloatValue, opts[ opt]->floatOpts, opts[ opt]->floatValues);
			memcpy( &combinedHelpMessage[ numOfCombinedHelps], &opts[ opt]->helpMessage[ posOfNextHelp[ opt]], numOfSrcHelps *sizeof( char*));
			posOfNextHelp[ opt] += numOfSrcHelps;
			numOfCombinedHelps += numOfSrcHelps;
		}
		for( int opt = 0; opt < count; ++opt)
		{
			int numOfSrcHelps = AppendOpts( combinedIntOpts, combinedIntValues, opts[ opt]->intOpts, opts[ opt]->intValues);
			memcpy( &combinedHelpMessage[ numOfCombinedHelps], &opts[ opt]->helpMessage[ posOfNextHelp[ opt]], numOfSrcHelps *sizeof( char*));
			posOfNextHelp[ opt] += numOfSrcHelps;
			numOfCombinedHelps += numOfSrcHelps;
		}
		for( int opt = 0; opt < count; ++opt)
		{
			int numOfSrcHelps = AppendOpts( combinedInt64Opts, combinedInt64Value, opts[ opt]->int64Opts, opts[ opt]->int64Values);
			memcpy( &combinedHelpMessage[ numOfCombinedHelps], &opts[ opt]->helpMessage[ posOfNextHelp[ opt]], numOfSrcHelps *sizeof( char*));
			posOfNextHelp[ opt] += numOfSrcHelps;
			numOfCombinedHelps += numOfSrcHelps;
		}
		for( int opt = 0; opt < count; ++opt)
		{
			if( opts[ opt]->paramOpts)
			{// This one has parameters
				int numOfSrcHelps = strli( opts[ opt]->paramOpts);
				strcat( combinedParamOpts, opts[ opt]->paramOpts);	// Best if only ONE entry, the last one, has any
				memcpy( &combinedHelpMessage[ numOfCombinedHelps], &opts[ opt]->helpMessage[ posOfNextHelp[ opt]], numOfSrcHelps *sizeof( char*));
				posOfNextHelp[ opt] += numOfSrcHelps;
				numOfCombinedHelps += numOfSrcHelps;
			}
		}
		return	GetAllOpts( argc, argv,
			combinedBoolOpts, combinedBoolValues,
			combinedStrOpts, combinedStrValue,
			combinedFloatOpts, combinedFloatValue,
			combinedIntOpts, combinedIntValues,
			combinedInt64Opts, combinedInt64Value,
			combinedParamOpts, combinedHelpMessage);
	}
}

void EnableVT100Console()
{
#if WINCODE
	// Set output mode to handle virtual terminal sequences
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	Test( hOut);
	DWORD dwMode = 0;
	Test( (bool) GetConsoleMode( hOut, &dwMode));
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	Test( (bool) SetConsoleMode( hOut, dwMode));
#endif
}

static int UTF8ByteCount( int prefixChar)
{// Byte coubnt of UTF8 character based on prefix
/*
				  UTF8 table
   Char. number range  |		UTF-8 octet sequence
	  (hexadecimal)	   |			  (binary)
   --------------------+---------------------------------------------
   0000 0000-0000 007F | 0xxxxxxx
   0000 0080-0000 07FF | 110xxxxx 10xxxxxx
   0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
   0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

   for (p; *p != 0; ++p)
	count += ((*p & 0xc0) != 0x80);
*/
	if( (prefixChar & 0b11100000) == 0b11000000 )
		return 2;
	if( (prefixChar & 0b11110000) == 0b11100000 )
		return 3;
	if( (prefixChar & 0b11111000) == 0b11110000 )
		return 4;
	return 1;
}

bool Utf8IsZWJ( const char* startByte)
{// I apolgize for the redundancy, all variation selectors are evil.  We delete them all.
	//	This catches red heart and other "variation selector" tags
	const unsigned char* sb = (unsigned char *) startByte;
	if( (sb[ 0] == 0xE2) && (sb[ 1] == 0x80) && (sb[ 2] == 0x8D))
	{// Zero width joiner
		return true;
	}
	return false;
}

bool Utf8IsEvilVariationSelector( const char* startByte)
{// I apolgize for the redundancy, all variation selectors are evil.  We delete them all.
	//	This catches red heart and other "variation selector" tags
	if( sTerminalHasCursorBugs)
	{
	const unsigned char* sb = (unsigned char *) startByte;
		static bool eatNextCharacter = false;		// Awful hack that assumes we get called again right away after seeing a ZJW.
	
	if( eatNextCharacter)
	{// Character following ZWJ
		eatNextCharacter = false;
		return true;
	}
	
	if( (sb[ 0] == 0xE2) && (sb[ 1] == 0x80) && (sb[ 2] == 0x8D))
	{// Zero width joiner
		eatNextCharacter = true;
		return true;
	}
	
	if( (sb[ 0] == 0xEF) && (sb[ 1] == 0xB8) && ((sb[ 2] & 0xF0) == 0x80))
		return true;	// Variation selector

	//	This catches skin tone modifiers (U+1F3FB–U+1F3FF)
	//	  For these codes, the UTF8 conversion is
	//	  0x00010000 - 0x001FFFFF:
	//	  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

	if( (sb[ 0] == 0xF0) && (sb[ 1] == 0x9F) && (sb[ 2] == 0x8F) && (sb[ 3] >= 0xBB) && (sb[ 3] <= 0xBF))
		return true;
	}
	return false;
}

static int UTF8TotalGlyphLength( uint8_t* buf, int pos, int len, int* startPos)
{ // Given position, hunt for the glyph edges and return them
/*
		First we need to find the beginning by baking up over ZWJs, variation selectors, and skin tone modifiers.
*/
	while( (pos > 0) && (buf[ pos] & 0xc0) == 0x80) --pos;	// Find the start of the character we're on
	
	for(;;)
	{// Every time we back up a character we start over
		uint8_t*sb = &buf[ pos];	// Make the next line easier to read
		if( pos >= 3 && (sb[ -3] == 0xE2) && (sb[ -2] == 0x80) && (sb[ -1] == 0x8D))
		{// Predecessor is zero width joiner, back up another character
			do { --pos;} while( (pos > 0) && (buf[ pos] & 0xc0) == 0x80);
			continue;
		}
		if( pos + 3 <= len && (sb[ 0] == 0xE2) && (sb[ 1] == 0x80) && (sb[ 2] == 0x8D))
		{// Zero width joiner, back up another character
			do { --pos;} while( (pos > 0) && (buf[ pos] & 0xc0) == 0x80);
			continue;
		}
		if( pos + 4 <= len && (sb[ 0] == 0xF0) && (sb[ 1] == 0x9F) && (sb[ 2] == 0x8F) && (sb[ 3] >= 0xBB) && (sb[ 3] <= 0xBF))
		{// Skin tone modifier, back up another character
			do { --pos;} while( (pos > 0) && (buf[ pos] & 0xc0) == 0x80);
			continue;
		}
		if( pos + 3 <= len && (sb[ 0] == 0xEF) && (sb[ 1] == 0xB8) && ((sb[ 2] & 0xF0) == 0x80))
		{// Variation selector, back up another character
			do { --pos;} while( (pos > 0) && (buf[ pos] & 0xc0) == 0x80);
			continue;
		}
		break;
	}
	*startPos = pos;	//Now we know where to start, go forward now until we reach the end
	
	pos += UTF8ByteCount( buf[ pos]);
	
	while( pos < len)
	{// Every time we go forward character we start over
		uint8_t*sb = &buf[ pos];	// Make the next line easier to read
		if( pos + 4 <= len && (sb[ 0] == 0xF0) && (sb[ 1] == 0x9F) && (sb[ 2] == 0x8F) && (sb[ 3] >= 0xBB) && (sb[ 3] <= 0xBF))
		{// Skin tone modifier, go forward another character
			do { ++pos;} while( (pos < len) && (buf[ pos] & 0xc0) == 0x80);
			continue;
		}
		if( pos + 3 <= len && (sb[ 0] == 0xEF) && (sb[ 1] == 0xB8) && ((sb[ 2] & 0xF0) == 0x80))
		{// Variation selector, go forward another character
			do { ++pos;} while( (pos < len) && (buf[ pos] & 0xc0) == 0x80);
			continue;
		}
		if( pos + 3 <= len && (sb[ 0] == 0xE2) && (sb[ 1] == 0x80) && (sb[ 2] == 0x8D))
		{// Zero width joiner, go forward TWO characters
			do { ++pos;} while( (pos < len) && (buf[ pos] & 0xc0) == 0x80);
			pos += UTF8ByteCount( buf[ pos]);
			if( pos > len)
				pos = len;
			continue;
		}
		break;
	}
	return pos - *startPos;
}

// A very simplified function to decode a UTF-8 character array
// into its UTF-32 code points.
// Error handling and full multi-byte sequence handling are omitted for brevity.
intptr_t decode_utf8_to_utf32( uint32_t *utf32Ptr, const uint8_t* utf8_ptr)
{

	uint32_t *startPos = utf32Ptr;		//So we can count sizes at the end
	while( *utf8_ptr)
	{// For every byte in the UTF8 string
		uint8_t byte1 = *utf8_ptr++;
		int kind = UTF8ByteCount( byte1);
		switch( kind)
		{
		case 1:
			*utf32Ptr++ = byte1;
			break;
		case 2:
			{
				uint8_t byte2 = *utf8_ptr++;
				*utf32Ptr++ = ((byte1 & 0x1F) << 6) | (byte2 & 0x3F);
			}
			break;
		case 3:
			{
				uint8_t byte2 = *utf8_ptr++;
				uint8_t byte3 = *utf8_ptr++;
				*utf32Ptr++ = ((byte1 & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F);
			}
			break;
		case 4:
			{
				uint8_t byte2 = *utf8_ptr++;
				uint8_t byte3 = *utf8_ptr++;
				uint8_t byte4 = *utf8_ptr++;
				*utf32Ptr++ = ((byte1 & 0x07) << 18) | ((byte2 & 0x3F) << 12) | ((byte3 & 0x3F) << 6) | (byte4 & 0x3F);
				break;
			}
		default:
			*utf32Ptr++ = 0xFFFD; // Replacement character for invalid UTF-8
		}
	}
	*utf32Ptr = 0;
	return (utf32Ptr - startPos);
}

EditableCommandLine::EditableCommandLine( const char* promptp)
:
	prompt( promptp)
{
}

void EditableCommandLine::DisplayPrompt()
{
	char ch = cmdLine[ pos];
	printf( "\r\033[K%s ", prompt);
	if( pos)
	{
		cmdLine[ pos] = 0;
		printf( "%s", cmdLine);
		cmdLine[ pos] = ch;
}
	if( ch)
	{// We're not at the end, print the rest of the command line around save/restore cursor
		printf( "\033" "7%s\033" "8", &cmdLine[ pos]);
	}
	fflush( stdout);
}

bool Utf8IsWide( const char* startByte)		// Returns true it a character MIGHT be two spaces wide in the terminal
{// This one returns true for everything that is more than 2 bytes of Unicode, which
 // is probably wrong from asian languages but seems to be good enough for emoji.
 // Safer would be to test against '1', which would cover a greater range but cost some performance
 // when character really aren't wide. Until I find a better way,
 // this will have to do.
	int byteCount = UTF8ByteCount( *startByte);
	return (byteCount > 2);
}


char* EditableCommandLine::ProcessOneCharacter( int readCh)
{
/*
				  UTF8 table
   Char. number range  |		UTF-8 octet sequence
	  (hexadecimal)	   |			  (binary)
   --------------------+---------------------------------------------
   0000 0000-0000 007F | 0xxxxxxx
   0000 0080-0000 07FF | 110xxxxx 10xxxxxx
   0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
   0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

   for (p; *p != 0; ++p)
	count += ((*p & 0xc0) != 0x80);
*/
	int cmdLen = strli( cmdLine);
	if( utfBytesLeft == 0)
	{// Not building a UTF string, these are the same
		utfPos = pos;
	}
	switch( readCh)
	{
	case 0: // Timeeout.  Neeed a veresion of ScopedGetch that doesn't time out
		break;
	case kLeftArrow:
		if( pos > 0)
		{
			int glyphLen = UTF8TotalGlyphLength( (uint8_t*) cmdLine, pos-1, cmdLen, &pos);
			if( glyphLen > 2)
				DisplayPrompt();	// Might be a double wide
			else
			{
				printf( "\033[D"); fflush( stdout);
			}
		}
		break;
	case kRightArrow:
		if( pos < cmdLen)
		{
			int glyphLen = UTF8TotalGlyphLength( (uint8_t*) cmdLine, pos, cmdLen, &pos);
			pos += glyphLen;
			if( glyphLen > 2)
				DisplayPrompt();	// Might be a double wide
			else
			{
				printf( "\033[C"); fflush( stdout);
			}
		}
		break;
	case kDownArrow:
		if( arrowPos < cmdLineHistory.size())
		{
			++arrowPos;
			if( arrowPos < cmdLineHistory.size())
				strncpy( cmdLine, cmdLineHistory[ arrowPos].c_str(), len);
			else
				strncpy( cmdLine, cmdLineHistory[ arrowPos].c_str(), len);
			cmdLen = strli( cmdLine);
			pos = cmdLen;
			printf( "\n%s %.*s\033" "7%s\033" "8", prompt, pos, cmdLine, &cmdLine[ pos]); fflush( stdout);
		}
		break;
	case kUpArrow:
		if( arrowPos > 0)
		{// Can't do anythnig if already at the top
			if( arrowPos < cmdLineHistory.size())
			{// Save this out to come back to it on downarrow if we don't take any of the previous commands
				temporaryLastLine = cmdLine;
			}
			--arrowPos;
			strncpy( cmdLine, cmdLineHistory[ arrowPos].c_str(), len);
			pos = strli( cmdLine);
			cmdLen = strli( cmdLine);
			printf( "\n%s %.*s\033" "7%s\033" "8", prompt, pos, cmdLine, &cmdLine[ pos]); fflush( stdout);
		}
		break;
	case kDelFwd:
		if( pos < cmdLen)
		{
			int glyphLen = UTF8TotalGlyphLength( (uint8_t*) cmdLine, pos, cmdLen, &pos);
			memcpy( &cmdLine[ pos], &cmdLine[ pos+glyphLen], len - pos - glyphLen);
			printf( "\033" "7\033[K%s\033" "8", &cmdLine[pos]); fflush( stdout);
		}
		break;
	case 127:	// Delete
	case 8:		// backspace (delete on Windows)
		if( pos > 0)
		{
			int glyphLen = UTF8TotalGlyphLength( (uint8_t*) cmdLine, pos-1, cmdLen, &pos);
			memcpy( &cmdLine[ pos], &cmdLine[ pos+glyphLen], len - pos - glyphLen);
			if( glyphLen > 2)
				DisplayPrompt();	// Might bee a double wide
			else
			{
			   printf( "\033[D\033" "7\033[K%s\033" "8", &cmdLine[pos]); fflush( stdout);
			}
		}
		break;
	case 10:
	case 13: // Allow tetminate on either.	Clients should expect blank lines if input uses crlf
		printf( "\n"); fflush( stdout);
		if( *cmdLine)
		{// Something to do, remember this.	 (We don't remember blank lines, but do return them)
			cmdLineHistory.push_back( cmdLine);
			pos = 0;
			arrowPos = cmdLineHistory.size();		// Reset to last command
		}
		return cmdLine;
		break;
	case 'R'-64:
		printf( "\n");
		DisplayPrompt();
		break;
	default:
		memcpy( &cmdLine[ pos+1], &cmdLine[ pos], len - pos);
		++cmdLen;	   // We cached string length earlier
		cmdLine[ pos++] = (char) readCh;
		if( utfBytesLeft)
		{// Working off a UTF8 thing
			--utfBytesLeft;
		}
		else if( UTF8ByteCount( readCh) != 1)
		{// Starting a UTF8 thing
			utfBytesLeft = UTF8ByteCount( readCh) - 1;
		}
		if( utfBytesLeft == 0)
		{// Reached the end of this UTF8 multi-byte - or is just one byte
			if( Utf8IsEvilVariationSelector( &cmdLine[ utfPos]))
			{// MacOS terminal can't display skin tone modifiers correctly, skip them
				memcpy( &cmdLine[ utfPos], &cmdLine[ pos], len - pos);
				pos = utfPos;
				break;
			}
			if( Utf8IsZWJ( &cmdLine[ utfPos]))
			{// Don't try to display a ZWJ if there is nothing following it yet
				break;
			}
			if( utfPos > 3 && Utf8IsZWJ( &cmdLine[ utfPos-3]))
			{// Preceesded by ZJW, do entire prompt
				DisplayPrompt();
			}
			else if( cmdLen == pos)
			{// End of string, just display the character
				printf( "%s", &cmdLine[ utfPos]);
				fflush( stdout);
			}
			else if( UTF8ByteCount( cmdLine[ utfPos]) > 2)
			{// Not sure of character width, do entire prompt
				DisplayPrompt();
			}
			else
			{// Character width is one, display it and back up a letter
				printf( "\033" "7%s\033" "8\033[C", &cmdLine[ utfPos]);
				fflush( stdout);
			}
		}
		break;
	}
	return nullptr;
}

std::vector<std::string> EditableCommandLine::cmdLineHistory;

/*
		Command line with editing capability
 */
void ConsoleGetLine( const char* prompt, char* buffer, int len)
{// Should probably move to ConsoleThings
	if( AmIBeingDebugged() || !isatty(STDIN_FILENO))
	{// Not a terminal window, use a regular read from stdin
		if( feof( stdin))
		{
			buffer[0] = 0;
		}
		else
		{
			printf( "%s", prompt); fflush( stdout);
			fgets( buffer, (int)len, stdin);
			buffer[ strlen( buffer) - 1] = 0;
		}
	}
	else
	{// Real terminal window, allow full editing
		EditableCommandLine cmdLine( prompt);
		cmdLine.DisplayPrompt();
		ScopedGetch gch;
		for( ;;)
		{
			int ch = gch.ReadCmd();
			if( ch == 0)
				continue;	// Need a version of ReadCmd that doesn't time out
			char* result = cmdLine.ProcessOneCharacter( ch);
			if( result)
			{
				strncpy( buffer, result, len);
				*result = 0;
				break;
			}
		}
	}
}
