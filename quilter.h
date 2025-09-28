//
//  quilter.h
//  quilter
//
//  Created by Jeffrey Lomicka on 5/1/25.
//  Copyright © 2025 Jeffrey Lomicka. All rights reserved.
//

#ifndef quilter_h
#define quilter_h
#include <stdio.h>
#include <string>
#include "JeffSema.h"

class AsyncHelper
{// A running asynchronous task
public:
	static JeffSemaphore iCompletionSema;		// "post" this when you want Execute to run.
	bool iReadyToExecute = false;				// Set this to true when you want Execute to run.
	bool iNeedNewPrompt = false;				// If you printf anything, you should request a new command line prompt
	char iDescription[ 512];					// Everyone needs one
    int iExecuteCounter = 0;                    // Number of times Execute has been called
    char iTimeStringMemory[ 32];

    virtual void DisplayPrompt();  // This is a hack until I make a base class for command lines
    int result = 0;                // When set to 2, it means we're exiting. This is also a hack until I make a base class for command lines
	virtual void Startup() = 0;
	virtual void Execute() = 0;
	virtual void Shutdown() = 0;
	virtual const char* Description()
	{// Default can be overridden
		return iDescription;
	}
	virtual ~AsyncHelper();
#if MACCODE
    int Printf( const char* formatString, ...) __printflike(2, 3);
#else
	int Printf(const char* formatString, ...);
#endif
    char* NowString();
};

// Utility functions for quilters - some of these might want to move into ConsoleThings or xraise?

size_t ReadFile(FILE *fp, char **buf);
void AsyncGetLine( char* buffer, size_t len, FILE* f, JeffSemaphore &completionSema, bool* ready);
void FGetLine( char* buffer, size_t len, FILE* f);
void AddActivity( AsyncHelper* newActivity);	// Adds to list of asynchronous activities
std::string DocumentsPath();
std::string SettingsPath();
std::string ActivityUniqueString();

enum DefaultValues
{
	AllowProblematicUnicode = 0,
	unknownSetting,
	MaxDefaultValues
};

const char* GetDefaultValue( enum DefaultValues selection);	// Returns reference to internal memory
void SetDefaultValue( enum DefaultValues selection, const char* newValue);

#endif /* quilter_h */
