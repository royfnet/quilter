#include <stdio.h>
#include "ConsoleThings.h"
#include <time.h>
#include <filesystem>
#include "TinyXML.hpp"
#include "quilter.h"
#if MACCODE
#include <unistd.h>
#include <sysdir.h>  // for sysdir_start_search_path_enumeration
#include <sys/sysctl.h>
#include <glob.h>    // for glob needed to expand ~ to user dir
#endif
#if WINCODE
#define _TIMESPEC_DEFINED       // pthread.h needs this on Windows
#include <pthread.h>
#include <io.h>
#endif

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

std::string DocumentsPath()
{// Returns path to user's documents
#if MACCODE
    char path[PATH_MAX];
    auto state = sysdir_start_search_path_enumeration(
        SYSDIR_DIRECTORY_DOCUMENT,
        SYSDIR_DOMAIN_MASK_USER);
    Test( (bool) (0 != sysdir_get_next_search_path_enumeration(state, path)));
    glob_t globbuf;
    Test(glob(path, GLOB_TILDE, nullptr, &globbuf));
    std::string result(globbuf.gl_pathv[0]);
    globfree(&globbuf);
    return result;
#else
	std::string retval(getenv("HOMEPATH"));
	retval = retval + "\\Documents\\";
	return retval;
#endif
}

std::string SettingsPath()
{// Returns path to user's application support folder
#if MACCODE
    char path[PATH_MAX];
    auto state = sysdir_start_search_path_enumeration(
        SYSDIR_DIRECTORY_APPLICATION_SUPPORT,
        SYSDIR_DOMAIN_MASK_USER);
    Test( (bool) (0 != sysdir_get_next_search_path_enumeration(state, path)));
    glob_t globbuf;
    Test(glob(path, GLOB_TILDE, nullptr, &globbuf));
    std::string result(globbuf.gl_pathv[0]);
    globfree(&globbuf);
    return result;
#else
	std::string retval(getenv("HOME"));
	retval = retval + "\\Documents\\";
	return retval;
#endif
}

static std::vector<AsyncHelper*> activities;		// Global list of things to do
static int uniqueActivityCount = 0;

void AddActivity( AsyncHelper* newActivity)
{// Exported fuction for commands that add themselves to the activity list
	activities.push_back( newActivity);
}

int AsyncHelper::Printf( const char* formatString, ...)
{// A version of printf for use in asynchronous helpers the triggers the dispaly of a new command prompt
    va_list args;
    va_start( args, formatString );
    int retval = vprintf( formatString, args);
    if( retval > 0)
        iNeedNewPrompt = true;
    va_end( args );
    return retval;
}

char* AsyncHelper::NowString()
{// Current time in a buffer unique to each helper
	time_t now; time( &now);
#if MACCODE
	struct tm nowInfo;
	localtime_r( &now, &nowInfo);
#endif
#if WINCODE
	struct tm nowInfo;
	localtime_s( &nowInfo, &now);
#endif
	strftime( iTimeStringMemory, sizeof(iTimeStringMemory), "%x %R", &nowInfo);
	return iTimeStringMemory;
}

std::string ActivityUniqueString()
{
    ++uniqueActivityCount;
    char scrap[ 10];
    snprintf( scrap, CountItems( scrap), "%06d", uniqueActivityCount);
    return scrap;
}

/*
		Read entire file in one fell swoop
*/
size_t ReadFile(FILE *fp, char **buf)
{// Reads a file into a new (malloc) buffer, returns length read.
	static const size_t readBufSize = 4096;

	size_t readBufAllocation = 0;	// Total allocation
	char* b = nullptr;				// Read buffer to return
	size_t readLen = 0;				// Size of most recent read
	size_t readSoFar = 0;
	while( readBufAllocation == readSoFar)
	{
		readBufAllocation += readBufSize;
		b = (char*) realloc( b, readBufAllocation + 1);	// +1 for terminating nul
		readLen = fread( &b[ readSoFar], 1, readBufSize, fp);
		readSoFar += readLen;
	}
	b[ readSoFar] = 0;	// Terminating nul
	*buf = b;
	if( readSoFar == 0)
		Test( nullptr);
	return readSoFar;
}
/*
		fgets, but accepts any line terminator
 */
void FGetLine( char* buffer, size_t len, FILE* f)
{
	int pos = 0;
	while( pos < len)
	{
		if( feof( f))
		{
			buffer[0] = 0;
			break;
		}
		char ch = getc( f);
		if( ch == 10 || ch == 13)
		{// Allow tetminate on either.  Clients should expect blank lines if input uses crlf.
			buffer[ pos] = 0;
			break;
		}
		buffer[ pos++] = ch;
	}
}

static const char* AllDefaultValues[ MaxDefaultValues] = {nullptr};

const char* GetDefaultValue( enum DefaultValues selection)
{// Default values for various things that are installation dependent
	if( !AllDefaultValues[ selection])
	{
		std::string retval;
		switch( selection)
		{
		case AllowProblematicUnicode: retval = "false"; break;
		case unknownSetting: retval = DocumentsPath(); break;
		case MaxDefaultValues:
		default: xraise( "Invalid default value selection", "int selection", (int) selection, nullptr); break;
		}
		char* result = new char[ retval.length()+1];
		strncpy( result, retval.c_str(), retval.length()+1);
		AllDefaultValues[ selection] = result;
	}
	return AllDefaultValues[ selection];
}	// Returns reference to internal memory

void SetDefaultValue( enum DefaultValues selection, const char* newValue)
{
	if( selection == AllowProblematicUnicode)
	{// Special case
		switch( *newValue)
		{
		case 't':
		case 'y':
		case 'T':
		case 'Y':
			sTerminalHasCursorBugs = false;
			newValue = "true";
			break;
		default:
			sTerminalHasCursorBugs = true;
			newValue = "false";
		}
	}
	delete[] AllDefaultValues[ selection];
	size_t newSize = strlen( newValue)+1;
	char* result = new char[ newSize];
	strncpy( result, newValue, newSize);
	AllDefaultValues[ selection] = result;
}

int SetCmd( CommandProc* cur)
{
	static const char* defaultValueNames[] =
	{
		"AllowProblematicUnicode",
		"UnknownSetting"
	};

	bool quiet = false;
	const char* boolOpts = "q";
	bool* boolValues[] = {&quiet};
    static const char* helps[] =
    {
		"Quiet, Omit list of current values",
        "Set default value. Lists all values."
    };

	int paramIndex = GetAllOpts(
        cur->iArgc, cur->iArgv,
        boolOpts, boolValues,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        "*", helps);
	
	for( ;paramIndex+1 < cur->iArgc; paramIndex += 2)
	{// Set in pairs
		int selection = 0;
		int matchCount = 0;
		int matchLen = (int) strlen( cur->iArgv[ paramIndex]);
		std::string matches;
		for( int sel = 0; sel < CountItems( defaultValueNames); ++sel)
		{// search for unique abbreviation
			if( strncasecmp( defaultValueNames[ sel], cur->iArgv[ paramIndex], matchLen) == 0)
			{// Matched this one
				selection = sel;
				if( matches.length() != 0) matches += ", ";
				matches += defaultValueNames[ sel];
				++matchCount;
			}
		}
		if( matchCount == 0)
		{
			xraise( "Matched no defaults names.", "str badName", cur->iArgv[ paramIndex], nullptr);
		}
		if( matchCount > 1)
		{
			xraise( "Ambiguous default name", "str possibleMatches", matches.c_str(), nullptr);
		}
		SetDefaultValue( (enum DefaultValues) selection, cur->iArgv[ paramIndex+1]);
	}
	
	if( !quiet) for( int sel = 0; sel < CountItems( defaultValueNames); ++sel)
	{// Display all values
		printf( "%s = \"%s\"\n", defaultValueNames[ sel], GetDefaultValue( (enum DefaultValues) sel));
	}
	
	return SUCCESS;
}

class AsyncReader
{// support for AsyncGetLine
public:
    char* iBuffer;
    size_t iLen;
    FILE* iFile;
    JeffSemaphore &iCompletionSema;
    bool* iReady;

    AsyncReader( JeffSemaphore &completionSema, bool *ready, char* buffer, size_t len, FILE* f)
    :
        iBuffer( buffer), iLen( len), iFile( f), iCompletionSema( completionSema), iReady( ready)
    {
		memset( iBuffer, 0, iLen);
		static size_t sNiceStackSize = 1024*16;
		pthread_t threadID;
		pthread_attr_t pa;
		Test( pthread_attr_init( &pa));
		size_t ss;    // Default stack size of 512K is too small for fwrite()
		Test( pthread_attr_getstacksize( &pa, &ss));
		if( ss < sNiceStackSize)
		{
			ss = sNiceStackSize;
			Test( pthread_attr_setstacksize( &pa, ss));
		}
		Test( pthread_attr_setdetachstate( &pa, PTHREAD_CREATE_DETACHED));
		Test( pthread_create( &threadID, &pa, AsyncReader::ReadThread, this));
		Test( pthread_attr_destroy( &pa));
	}
    
    void* ReadThreadGuts()
    {
        iBuffer[ 0] = 0;
        FGetLine( iBuffer, iLen, iFile);
        *iReady = true;
        iCompletionSema.Increment();
        return( nullptr);
    }

    static void* ReadThread( void* context)
    {// Wrapper given to phread.
        AsyncReader* sd = (AsyncReader*) context;
        sd->ReadThreadGuts();
        delete sd;
        return nullptr;
    }
};

void AsyncGetLine( char* buffer, size_t len, FILE* f, JeffSemaphore &completionSema, bool* ready)
{
    new AsyncReader( completionSema, ready, buffer, len, f);
}

int SplitCommandLine( char* commandLine, int *argc, char** argv, size_t argvsize)
{// commandLine is edited, and argv is returned as references into it, should be in consolethings.cpp
	*argc = 0;
	--argvsize;		// This lets us compare before we overrun the size
	char* pos = commandLine;
	for(;;)
	{// For each character in the command line
		while( *pos > 0 && *pos <= ' ') ++pos;	// Skip whitespace
		if( *argc == 0 && *pos == '@')
		{// "@" as first token , special case for running command files
			argv[ (*argc)++] = (char*) "@";	// Use our own "@" so we an nul-termiante it
			++pos;
		}
		if( *pos == 0) break;
		if( *argc >= argvsize)
			xraise( "Too many parameters on command", "int limit", argvsize, nullptr);
		argv[ (*argc)++] = pos;	// Parameter begins here
		char* outpos = pos;		// Removes quotes and quote characters as it goes
		while( *pos < 0 || *pos > ' ')
		{// Keep non-white-space characters, while deleting and processing quoting characters \, ", and ',
			if( *pos == '\\')
			{// Quoting next character, could be space or tab, or backslash
				if( *++pos == 0) break;
				*outpos++ = *pos++;			// Keeping a "\" quoted character
			}
			else if( *pos == '"' || *pos == '\'')
			{// quoted string, skip over that
				char q = *pos++;
				for(;;)
				{// While inside quote
					if( *pos == 0) break;
					else if( *pos == q)
					{// close quote, skip over that without keeping it
						++pos;
						break;
					}
					else if( *pos == '\\')
					{// Could be quoting a quote
						if( *++pos == 0) break;
						*outpos++ = *pos++;		// Keeping a "\" quoted character
					}
					else *outpos++ = *pos++;	// Keeping a chacater inside quotes
				}
			}
			else *outpos++ = *pos++;			// Keeping this non-whitespace character
		}
		if( *pos != 0) ++pos;
		*outpos = 0;	// Truncate the output string we are skipping over
	}
	return *argc;
}

int JsonCmd( CommandProc* cur)
{
    static const char* helps[] =
    {
        "Read JSON file to write out as XML."
    };

	int paramIndex = GetAllOpts(	// No options, just the one parameter
        cur->iArgc, cur->iArgv,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        "S", helps);

	char* jsonbuf = nullptr;
	FILE *infile = fopen( cur->iArgv[ paramIndex], "r");
	TestMsg( infile, "Can't open input file");
	ReadFile( infile, &jsonbuf);
	fclose( infile);
	TinyXml json;
	json.InitializeFromJSON( jsonbuf);
	WriteOut( &json, stdout);
	return cur->iFromCommandLine ? 2 : 0;
}

int Help( CommandProc *cur);
int RunCmd( CommandProc *cur);

int ExitCmd( CommandProc* cur)
{
    static const char* helps[] =
    {
        "Terminates all activities and exits."
    };
	int paramIndex = GetAllOpts(
        cur->iArgc, cur->iArgv,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        "", helps);
    if( cur->iArgc > paramIndex)
        xraise( "Exit takes no parameters", nullptr);
	return 2;	// Returning 2 tells command line to exit.
}

int EchoCmd( CommandProc* cur)
{
	static const char* helps[] =
	{
		"Echos parameters back to you."
	};

	int paramIndex = GetAllOpts(	// No options, just the one parameter
        cur->iArgc, cur->iArgv,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        ".", helps);
	for(int i = paramIndex; i < cur->iArgc; ++i)
	{
		printf( "Paraemter %d is \"%s\"\n", i-paramIndex+1, cur->iArgv[ i]);
	}
    //	If we came from the command line, we exit now by returning 2
	return cur->iFromCommandLine ? 2 : 0;
}

int TestCmd( CommandProc* cur)
{
	static const char* helps[] =
	{
		"Echo back character codes in UTF8 and UTF32."
	};

	GetAllOpts(	// No options
        cur->iArgc, cur->iArgv,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        "", helps);

	printf( "wchar_t size is %d\n", (int) sizeof( wchar_t));
    for(;;)
    {
        char buf[ 512];
        uint32_t wbuf[ 512];
        ConsoleGetLine( "X to exit>", buf, 512);
        printf( "%s\n", buf);
        for( int c = 0; c < strlen( buf); ++c) printf( "0x%x ", (int)(unsigned char) buf[ c]);
        printf( "\n");
        decode_utf8_to_utf32( wbuf, (unsigned char*) buf);
        for( int c = 0; wbuf[c]!=0; ++c)
        {
            printf( "U-%x ", (unsigned int) wbuf[c]);
        }
        printf( "\n");
        if( buf[ 0] == 'X')
            break;
    }
	//If we came from the command line, we exit now by returning 2
    // Should do this with a lot more commands.
    return cur->iFromCommandLine ? 2 : 0;
}

static char const *cmds[] =
{
    "help",
    "exit",
    "echo",
    "json",
    "set",
    "test",
    "run",
    "@",
    NULL
};
static int (*rtns[])( CommandProc*) =
{
    Help,
	ExitCmd,
	EchoCmd,
    JsonCmd,
    SetCmd,
    TestCmd,
	RunCmd,
	RunCmd,
    NULL
};

int RunCmd( CommandProc* cur)
{// Runs a command file

	static const char* helps[] =
	{
		"Command file to run"
	};

	int paramIndex = GetAllOpts(	// No options, just the one parameter
        cur->iArgc, cur->iArgv,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        nullptr, nullptr,
        "S", helps);

	int result = 0;			// We return the result of the last command.  If it was exit, we exit
	const char* filename = cur->iArgv[ paramIndex];
	FILE* stdFile = fopen( filename, "rb");
	if( stdFile == nullptr) TestMsg( -1, filename);				// Errors out witth errno
	try
	{
		while( !feof( stdFile))
		{// For each line the input file
			char scrap[ 256] = {0};
			int ac = 0;
			char* av[64];
			fgets( scrap, CountItems( scrap), stdFile);
			SplitCommandLine( scrap, &ac, av, CountItems( av));
			if( ac > 0)
			{// this line isn't logically blank, dispatch on it
				CommandProc cur( ac, av);
				result = Dispatch( &cur, cmds, rtns);
				if( result == 2)
					break;
			}
		}
	}
	catch( ...)
	{
		fclose( stdFile);
		stdFile = nullptr;
		throw;
	}
	return result;
}

int Help( CommandProc *cur)
{
    if( cur->iArgc >= 2 && strcmp( cur->iArgv[ 1], helpflag) == 0)
    {// Help on ourself, don't recurse
		printf( "Version %s %s %s\n\n", versionString, __DATE__, __TIME__);
        printf( "help - displays help page.\n");
    }
    else
    {// Help on everyone else
    	cur->iArgv++;
    	cur->iArgc--;
        DoHelp( "", cur, cmds, rtns);   // Skip over the help command
    }
	return cur->iFromCommandLine ? 2 : 0;
}

AsyncHelper::~AsyncHelper()
{
}

void AsyncHelper::DisplayPrompt()
{// This is a hack until I make a base class for command lines
    printf( "Quilter>"); fflush( stdout);
}

class AsyncEditableCommandLine : public AsyncHelper, EditableCommandLine
{
public:
    int readCh = 'R'-64;         // Character reseult
    ScopedGetch* gch = nullptr;
    JeffSemaphore gchLock;      // Using it like a mutex
    bool exiting = false;
    
    AsyncEditableCommandLine()
    :
		EditableCommandLine( "Quilter>")
	{
	}
    
    void* ReadThreadGuts()
    {
        while( !exiting)
        {
            gchLock.Decrement();
            while( !exiting)
            {
                readCh = gch->ReadCmd();
                if( readCh != 0)
					break;
            }
            iReadyToExecute = true;
            iCompletionSema.Increment();
        }
        return( nullptr);
    }

    static void* ReadThread( void* context)
    {// Wrapper given to phread.
        AsyncEditableCommandLine* sd = (AsyncEditableCommandLine*) context;
        sd->ReadThreadGuts();
        return nullptr;
    }

    void Startup() override
    {
        exiting = false;
        gch = new ScopedGetch();
        gchLock.Increment();
        static size_t sNiceStackSize = 1024*16;
        pthread_t threadID;
        pthread_attr_t pa;
        Test( pthread_attr_init( &pa));
        size_t ss;    // Default stack size of 512K is too small for fwrite()
        Test( pthread_attr_getstacksize( &pa, &ss));
        if( ss < sNiceStackSize)
        {
            ss = sNiceStackSize;
            Test( pthread_attr_setstacksize( &pa, ss));
        }
        Test( pthread_attr_setdetachstate( &pa, PTHREAD_CREATE_DETACHED));
        Test( pthread_create( &threadID, &pa, AsyncEditableCommandLine::ReadThread, this));
        Test( pthread_attr_destroy( &pa));
    }

    virtual const char* Description() override
    {
        return "Editable command interpreter";
    }

    virtual void DisplayPrompt() override
    {
		EditableCommandLine::DisplayPrompt();
    }
    
    void Execute() override
    {// Process most recent charactere if there is one, and queue up reading the next
		char* cmdLine = ProcessOneCharacter( readCh);
		if( cmdLine)
		{
			delete gch;			// Run command line in normal mode
			gch = nullptr;
		    try
            {
                char* av[64];
                int ac = 0;
                if( *cmdLine)
                {// Something to do
                    SplitCommandLine( cmdLine, &ac, av, CountItems( av));
                    if( ac != 0)
                    {
                        CommandProc cur( ac, av);
                        result = Dispatch( &cur, cmds, rtns);
                    }
                }
            }
            catch( std::exception& err)
            {
                DisplayException( err);
            }
            catch( ...)
            {
                printf( "Don't know why it failed\n");
            }
            *cmdLine = 0;
            if( result == 2)
            {// Exiting, mark it so
				exiting = true;
           }
            else
            {
            iNeedNewPrompt = true;
			gch = new ScopedGetch();
		}
        }
        gchLock.Increment();        // Unlock read thread
    }
    
    void Shutdown() override
    {// Restartable shutdown
        exiting = true;
		gchLock.Increment();
    }
    
    ~AsyncEditableCommandLine()
    {// non-restartable shutdown
    }
};

class AsyncCommandLine : public AsyncHelper
{
public:
	char cmdLine[ 512] = {0};	// Command line read asynchrounously

	void Startup() override
	{
		AsyncGetLine( cmdLine, CountItems( cmdLine), stdin, iCompletionSema, &iReadyToExecute);
	}

	virtual const char* Description() override
	{
		return "Command interpreter";
	}


	void Execute() override
	{// Process most recent command if there is one, and queue up reading the next
		try
		{
			char* av[64];
			int ac = 0;

			if( *cmdLine)
			{// Something to do
				SplitCommandLine( cmdLine, &ac, av, CountItems( av));
				if( ac != 0)
				{
					CommandProc cur( ac, av);
					result = Dispatch( &cur, cmds, rtns);
				}
			}
		}
		catch( std::exception& err)
		{
			DisplayException( err);
		}
		catch( ...)
		{
			printf( "Don't know why it failed\n");
		}
		if( result != 2)
		{// Not exiting, set up next prompt
			iNeedNewPrompt = true;
			AsyncGetLine( cmdLine, CountItems( cmdLine), stdin, iCompletionSema, &iReadyToExecute);
		}
	}
	
	void Shutdown() override
	{// Restartable shutdown
	}
	
	~AsyncCommandLine()
	{// non-restartable shutdown
		fclose( stdin);	// This should error out the pending read and exit the thread
	}
};

JeffSemaphore AsyncHelper::iCompletionSema;

int main( int argc, char **argv)
{
	bool needPrompt = true;        // Goes true if any activity requsts a new prompt
/*
		We only have editable command lines when using a terminal window. Debugger window
		and I/O pipes don't count.
 */
	AsyncHelper* cmdLine = nullptr;
    if( AmIBeingDebugged() || !isatty(STDIN_FILENO))
		cmdLine =  new AsyncCommandLine() ;
    else
		cmdLine = new AsyncEditableCommandLine();
	cmdLine->Startup();
/*
		Take note that the LIST comnmand assumes cmdLine is first in activities, so you
		need to create this before running any command files or command lines.
*/
	activities.push_back( cmdLine);

//		Locate startup command file

	std::string cmdFileName = DocumentsPath() + "/QuilterStartup.cmd";
	if( std::filesystem::exists( cmdFileName))
	{// Startup command file exists, run it
		const char* runparams[3] = { "run", cmdFileName.c_str(), nullptr};
		try
		{
			CommandProc cur( 2, runparams);
			cmdLine->result = Dispatch( &cur, cmds, rtns);
		}
		catch( std::exception& err)
		{
			DisplayException( err);
			cmdLine->result = 1;		// non-fatal
		}
		catch( ...)
		{
			printf( "Don't know why it failed\n");
			cmdLine->result = 1;
		}
	}

	if( argc > 1)
	{// Actually got command line parameters, run as first command
		try
		{
			CommandProc cur( argc-1, &argv[1]);
			cur.iFromCommandLine = true;
			cmdLine->result = Dispatch( &cur, cmds, rtns);
		}
		catch( std::exception& err)
		{
			DisplayException( err);
			cmdLine->result = 2;		// Exits on error
		}
		catch( ...)
		{
			printf( "Don't know why it failed\n");
			cmdLine->result = 2;
		}
	}
	// open up and read from stdin
	while( cmdLine->result != 2)
	{
        if( needPrompt)
        {
            cmdLine->DisplayPrompt();
            needPrompt = false;
        }
		AsyncHelper::iCompletionSema.Decrement();
		for( int i = 0; i < activities.size(); ++i)
		{// Process all the handlers, including the first one that takes commands from the command line
			if( activities[ i]->iReadyToExecute)
			{// This one is ready to go, launch it
				activities[ i]->iReadyToExecute = false;	// Reset for next event
				try
				{
					activities[ i]->Execute();
					needPrompt = activities[i]->iNeedNewPrompt;
                    ++activities[i]->iExecuteCounter;
				}
				catch( std::exception& err)
				{
					DisplayException( err);
					activities[ i]->Shutdown();
					delete activities[ i];
					activities.erase( activities.begin() + i);
					--i;
					needPrompt = true;
				}
				catch( ...)
				{
					printf( "Don't know why %s failed\n", activities[ i]->Description());
					activities[ i]->Shutdown();
					delete activities[ i];
					activities.erase( activities.begin() + i);
					--i;
					needPrompt = true;
				}
                activities[i]->iNeedNewPrompt = false;
			}
		}
	}
/*
		Shut down everyone
 */
	while( activities.size())
	{// For all of our processes
		size_t victim = activities.size() - 1;
		activities[ victim]->Shutdown();
		delete activities[ victim];
		activities.pop_back();
	}
	return 0;
}

