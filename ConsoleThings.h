//
//  ConsoleThings.h
//
//  Copyright (c) 2025 Jeffrey Lomicka. All rights reserved.
//

#ifndef __audio__ConsoleThings__
#define __audio__ConsoleThings__
#include "xraise.h"
#include <time.h>
#include <vector>
#include <string.h>
#if MACCODE
#include <termios.h>
#endif
#if WINCODE
#include <string>
#endif

//      Some global utilities that throw on error
int xatoi( const char* s);
double xatof( const char* s);
int64 xatoint64( const char* s);
double ConvertAsciiToFloat( const char* s);	// Like xatof but also accepts hexadecimal
double PitchToFreq( const char* s);			// Like ConvertAsciiToFloat but also accepts musical notes
double ConvertTimeToSeconds( const char* curArg);	// accepts hh:mm:ss.sss but also falls back to PitchToFreq
time_t xatot( const char* datetimeString);	// Accepts date and time as yyyy-mm-dd hh:mm:ss
char* MakeCommandLine( int argc, const char * const argv[]);	// Returns a static buffer
void EnableVT100Console();
size_t decode_utf8_to_utf32( uint32_t *utf32Ptr, const uint8_t* utf8_ptr);

// Options interpreter with built-in help

int GetAllOpts(
	int argc, const char * const argv[],
	const char* boolOpts, bool* boolValues[],
	const char* strOpts, const char** strValues[],
	const char* floatOpts, double* floatValues[],
	const char* intOpts, int* intValues[],
	const char* int64Opts, int64* int64Values[],
	const char* paramOpts, const char* const helpMessages[]);          // must have as many of help messages are there are opts combined

// To handle common options, build one that concatenates all the paraemters

typedef struct AllOpts
{
	const char* boolOpts = nullptr; bool** boolValues = nullptr;
	const char* strOpts = nullptr; const char*** strValues = nullptr;
	const char* floatOpts = nullptr; double** floatValues = nullptr;
	const char* intOpts = nullptr; int** intValues = nullptr;
	const char* int64Opts = nullptr; int64** int64Values = nullptr;
	const char* paramOpts = nullptr; const char** helpMessage = nullptr;          // must have as many of help messages are there are opts combined
} AllOpts_t;

int GetAllOpts(		// Concatenates options
	int argc, const char * const argv[],
	AllOpts_t **opts,
	int count = 0);

// Variations

int GetAllOpts(
	int argc, const char * const argv[],
	const char* strOpts, const char** strValues[]);
int GetAllOpts(
	int argc, const char * const argv[],
	const char* boolOpts, bool* boolValues[],
	const char* strOpts, const char** strValues[]);

int GetAllOpts(
	int argc, const char * const argv[],
	const char* boolOpts, bool* boolValues[],
	const char* strOpts, const char** strValues[],
	const char* floatOpts, double* floatValues[]);

int GetAllOpts(
	int argc, const char * const argv[],
	const char* floatOpts, double* floatValues[],
	const char* intOpts, int* intValues[]);

int GetAllOpts(
	int argc, const char * const argv[],
	const char* boolOpts, bool* boolValues[],
	const char* strOpts, const char** strValues[],
	const char* floatOpts, double* floatValues[],
	const char* intOpts, int* intValues[]);

int GetAllOpts(
	int argc, const char * const argv[],
	const char* boolOpts, bool* boolValues[],
	const char* strOpts, const char** strValues[],
	const char* floatOpts, double* floatValues[],
	const char* intOpts, int* intValues[],
	const char* int64Opts, int64* int64Values[]);

int GetAllOpts(
	int argc, const char * const argv[],
	const char* strOpts, const char** strValues[],
	const char* floatOpts, double* floatValues[],
	const char* intOpts, int* intValues[]);

// Variations with params and help
int GetAllOpts(
	int argc, const char * const argv[],
	const char* strOpts, const char** strValues[],
	const char* paramOpts,const char* const helpMessages[]);

int GetAllOpts(
	int argc, const char * const argv[],
	const char* boolOpts, bool* boolValues[],
	const char* strOpts, const char** strValues[],
	const char* paramOpts,const char* const helpMessages[]);

int GetAllOpts(
	int argc, const char * const argv[],
	const char* boolOpts, bool* boolValues[],
	const char* strOpts, const char** strValues[],
	const char* floatOpts, double* floatValues[],
	const char* paramOpts,const char* const helpMessages[]);

int GetAllOpts(
	int argc, const char * const argv[],
	const char* boolOpts, bool* boolValues[],
	const char* strOpts, const char** strValues[],
	const char* intOpts, int* intValues[],
	const char* paramOpts,const char* const helpMessages[]);

int GetAllOpts(
	int argc, const char * const argv[],
	const char* boolOpts, bool* boolValues[],
	const char* strOpts, const char** strValues[],
	const char* floatOpts, double* floatValues[],
	const char* intOpts, int* intValues[],
	const char* paramOpts,const char* const helpMessages[]);

int GetAllOpts(
	int argc, const char * const argv[],
	const char* strOpts, const char** strValues[],
	const char* floatOpts, double* floatValues[],
	const char* intOpts, int* intValues[],
	const char* paramOpts,const char* const helpMessages[]);

int GetAllOpts(
	int argc, const char * const argv[],
	const char* floatOpts, double* floatValues[],
	const char* intOpts, int* intValues[],
	const char* paramOpts,const char* const helpMessages[]);

int GetAllOpts(
	int argc, const char * const argv[],
	const char* strOpts, const char** strValues[],
	const char* floatOpts, double* floatValues[],
	const char* paramOpts,const char* const helpMessages[]);

static const int kUpArrow=0x1b5b41;
static const int kDownArrow=0x1b5b42;
static const int kRightArrow=0x1b5b43;
static const int kLeftArrow=0x1b5b44;
static const int kDelFwd=0x031b5b7e;
extern bool sTerminalHasCursorBugs;

class ScopedGetch
{// Within this scope, reads to STDIN will be single character
#ifndef _WIN32
	struct termios noncon;
#endif
	char iTypeAhead[ 64];
	int iTypeAheadLength;
public:
	ScopedGetch();
	~ScopedGetch();
	size_t ReadBuf( char* buf, size_t len);
	int ReadChar();
	int ReadCmd();
	void ReadLine( 	// Reads in a whole line to typeahaed buffer, or provided buffer
		char* buf = nullptr,
		size_t buflen = 0);
};

void PrintAllNotes();	// Essentially a unit test for PitchToFreq

int64 CommaSeparatedListOfBits( const char* bitStr);    // String to integer for comma separated lists for numbers 1-64
const char** CommaSeparatedListOfValues( const char* valueStr, int* argcptr);	// Converts comma separate list of values into array of strings

bool DefaultFileName( char* givenFileName, char* fileNameBuffer, size_t bufSize, const char* type = ".wav");
char* FilenameWithChannel( char* nameBuffer, const char* baseFileName, const char* suffix, int channel, const char* type=nullptr);
char* GetFileFromList( char* fileNameBuffer, size_t bufLen, const char* startingFolder);
void ScrapeArgs( int* argcp, char ***argvp);        // Converts argv[0] into a new argv, if it is the only thing
bool AmIBeingDebugged(void);

/*
		Command parameter dispatching
*/

struct CommandProc
{
	int iArgc;
	const char* const *iArgv;
	bool iFromCommandLine = false;
	
	CommandProc(
		int argc,
		const char * const argv[])
	:
		iArgc( argc),
		iArgv( argv)
	{}
};

int Dispatch(		// Disptach on keyword
	CommandProc* cur,
	const char** cmds,
	int (** rtns)( CommandProc* cur));

extern const char* helpflag;

void DoHelp(
	const char* defaultCommand,	// Cammand to use if argc is 0, should never need to be used
	CommandProc* cur,
	const char** cmds,
	int (** rtns)( CommandProc* cur));

class EditableCommandLine
{
	char cmdLine[ 512] = {0};    // Command line read asynchrounously
	int readCh = 'R'-64;         // Character result
	static std::vector<std::string> cmdLineHistory;
	std::string temporaryLastLine;
	int arrowPos = (int) cmdLineHistory.size();           // Position in cmdLineHistory for uparrow/downarrow
	int pos = 0;            // Byte position to insert in cmdLine
	int utfBytesLeft = 0;   // Working out a UTF8 multibyte
	int utfPos = 0;         // Position of first byte of UTF8  multibyte
	int len = CountItems( cmdLine) - 1;  // Always leave the trailing nul
	const char* prompt = ">";

public:
	EditableCommandLine( const char* promptp);
	void DisplayPrompt();
	char* ProcessOneCharacter( int readCh);
};
/*
		Get a command line with history and editing, but also works in debugger of if stdin is redirected.
 */
void ConsoleGetLine( const char* prompt, char* buffer, size_t len);

#endif /* defined(__audio__ConsoleThings__) */
