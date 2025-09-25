//
//  TinyXML.cpp
//
//	A tiny XML parser suitable for reading iXML metadata in .WAV files.
//
//  Created by Jeffrey Lomicka on 7/7/18.
//  Copyright © 2018,2025 Jeffrey Lomicka. All rights reserved.
//

#include "xraise.h"
#include "TinyXML.hpp"

static bool sPreserveCase = false;

static char* SkipTokenStuff( char* pos)
{// Returns pointer to first character after the end of the token
	while( *pos
		&&
		(
			(*pos >= 'A' && *pos <= 'Z')
			||
			(*pos >= 'a' && *pos <= 'z')
			||
			(*pos >= '0' && *pos <= '9')
			||
			(*pos == '_')
			||
			(*pos == '?')
		)
	)
	{// Convert to upper case and move on
		if( !sPreserveCase) *pos = (char)toupper( *pos);
		++pos;
	}
	return pos;
}

static char* SkipQuotedString( char* pos)
{// pos should be pointing at the open quote, we use that as the quote character
 // Quotes are quoted with "\"
	char theQuote = *pos++;
	while( *pos && *pos != theQuote)
	{
		if( *pos == '\\' && pos[1] ==  theQuote)
			++pos;	// Skip quoted quotes
		++pos;
	}
	return pos;
}

static char* SkipEverythingUntil( char* pos, const char* delimiter)
{// Skips ahead to delimited given, but allows the delimiter within quotes
	size_t dlen = strlen( delimiter);
	while( *pos && strncmp( pos, delimiter, dlen) != 0)
	{
		if( *pos == '"' || *pos == '\'')
			pos = SkipQuotedString( pos);
		if( *pos) ++pos;
	}
	if( *pos)
	{	// Nullify and skip the delimiter
		*pos = 0;
		pos += strlen( delimiter);
	}
	return pos;
}

static char* SkipWhitespace( char* pos)
{// Returns pointer to first non-whitespace character
	while( *pos)
	{
		if( strncmp( pos, "<!--", 4) == 0)
			pos = SkipEverythingUntil( pos, "-->");
		else if( *pos > ' ')
			break;
		else
			++pos;
	}
	return pos;
}


static char* SkipEverythingUntil( char* pos, char delimiter)
{// Skips ahead to delimited given, but allows the delimiter within quotes
	pos = SkipWhitespace( pos);
	while( *pos && *pos != delimiter)
	{
		if( *pos == '"')
			pos = SkipQuotedString( pos);
		if( *pos) ++pos;
	}
	if( *pos) *pos++ = 0;	// Nullify and skip the delimiter
	return pos;
}

static char* SkipEverythingUntilButLeaveBehind( char* pos, char delimiter, char del2)
{// Skips ahead to delimited given, but allows the delimiter within quotes
	pos = SkipWhitespace( pos);
	while( *pos && *pos != delimiter && *pos != del2)
	{
		if( *pos == '"')
			pos = SkipQuotedString( pos);
		if( *pos) ++pos;
	}
	return pos;
}

bool SkipEverythingUntil( char* &pos, char delimiter1, const char* delimiter2)
{// Skips until the first one of the pair. "pos" is updated in place
	bool retval = false;		// True means we matched delimter2, False delimter1 or end of string
	size_t dlen = strlen( delimiter2);
	pos = SkipWhitespace( pos);
	while( *pos && *pos != delimiter1 && strncmp( pos, delimiter2, dlen) != 0)
	{
		if( *pos == '"')
			pos = SkipQuotedString( pos);
		if( *pos) ++pos;
	}
	if( *pos)
	{// Got one, terminate the thing we are parsing and skip over the delimiter
		retval = (strncmp( pos, delimiter2, dlen) == 0);	// True  means delimtier 2
		*pos = 0;	// Nullify and skip the delimiter
		if( retval) pos += dlen; else ++pos;
	}
	return retval;
}

static char* SkipComment( char* pos)
{// Returns pointer to first non-whitespace character after skipping a comment
	while( *pos)
	{
		if( strncmp( pos, "![CDATA[", 8) == 0)
			pos = SkipEverythingUntil( pos, "]]>");
		if( strncmp( pos, "!--", 3) == 0)
			pos = SkipEverythingUntil( pos, "-->");
		else if( *pos > ' ')
			break;
		else
			++pos;
	}
	return pos;
}

char* TinyXml::Initialize( char* pos, const char* parentKeyword /* == nullptr */)
{// Recursively initialize a TinyXML thing.
 // BEWARE! the caracter string passed IS modified, terminating nuls are scattered througought,
 // and must have a lifetime that exceeds this object, as we keep pointers into it.  It is NOT
 // copied.
	pos = SkipWhitespace( pos);
	pos = SkipEverythingUntil( pos, '<');
	while( *pos == '!')	// Should probably be all non-token characters
	{// A comment or a script.
		pos = SkipComment( pos);
		pos = SkipEverythingUntil( pos, '<');
	}
	pos = SkipWhitespace( pos);
	if( *pos == '/' || *pos == 0)
	{// We're finishing our parent's block
		return pos;
	}
	iMyKeyword = pos;
	pos = SkipTokenStuff( pos);
	char* keyworedEnd = pos;
	pos = SkipWhitespace( pos);
	iMyParameters = pos;
	iSelfTerminated = SkipEverythingUntil( pos, '>', "/>");	//Updates 'pos', handles self-terminated case
	*keyworedEnd = 0;

	#ifdef TOUCHSTONE
	if( parentKeyword && strcasecmp( parentKeyword, "tr") == 0 && strcasecmp( parentKeyword, iMyKeyword) == 0)
	{// Need to back up and end the parent, bug in touchstone HTML
		*keyworedEnd = '>';
		pos = iMyKeyword-1;
		*pos = -1;
		return pos;
	}

	if( !iSelfTerminated)
	{// Old style keywords that always self terminate (Touchstone)
		const char* selfies[] =
		{
			"meta",
			"link",
			"br"
		};

		for( int s = 0; s < CountItems( selfies); ++s)
		{// Check the list for selfies
			if( strcasecmp( iMyKeyword, selfies[ s]) == 0)
			{// Matched
				iSelfTerminated = true;
				break;
			}
		}
	}
	#endif
	
	if( iSelfTerminated)
	{
		pos = SkipWhitespace( pos);
	}
	iMyContent = pos;
	//printf( "Parsing %s\n", iMyKeyword);
/*
		Iterate filling in our tag list until we get our delimiter
*/
	for(;!iSelfTerminated;)
	{// For each tag you find
		TinyXml candidate;
		candidate.iMyIndex = 0;
		candidate.iMyKeyword = nullptr;
		pos = candidate.Initialize( pos, iMyKeyword);
		if( *pos == -1)
		{// Handles case where closers are omitted, and parents are closed by their children
			*pos = '<';
			break;
		}
		if( *pos == '/')
		{// This is our closer, or it at least SHOULD be
			pos = SkipWhitespace( ++pos);	// This is probably not required
			char* myCloser = pos;
			pos = SkipTokenStuff( pos);
			char* myCloserEnd = pos;
			pos = SkipWhitespace( pos);
			pos = SkipEverythingUntil( pos, '>');
			pos = SkipWhitespace( pos);
			*myCloserEnd = 0;
			if( strcasecmp( iMyKeyword, myCloser) != 0)
			{
				if( parentKeyword && strcasecmp( parentKeyword, myCloser) == 0)
				{// This is my parent's closer, mine is missing
					// Uncomment these fprintf's if you want malform XML diagnostics
					//fprintf( stderr, "Unbalenced tokens, %s closed by parent /%s\n", iMyKeyword, myCloser);
					*myCloserEnd = '>';	// Put this back
					return myCloser-1;	// Let parent close itself
				}
				//fprintf( stderr, "Unbalenced tokens, %s closed by /%s\n", iMyKeyword, myCloser);
			}
			break;
		}
		//	Figure out the index for this keyword by counting predecessors that match
		if( candidate.iMyKeyword)
		{// If *pos is nul, iMyKeyword may or may not have been set.
			for( std::vector<TinyXml*>::iterator it = iXmlContent.begin() ; it != iXmlContent.end(); ++it)
			{
				if( strcasecmp( (*it)->iMyKeyword, candidate.iMyKeyword) == 0)
					candidate.iMyIndex++;
			}
			iXmlContent.push_back( new TinyXml( candidate));
			candidate.iXmlContent.clear();
		}
		if( *pos == 0)
		{
			break;	// Okay, we're done
		}
	}
	return pos;
}

TinyXml::~TinyXml()
{
	while( iXmlContent.size())
	{
		auto temp = iXmlContent.back();
		delete temp;
		iXmlContent.pop_back();
	}
}

bool CompareKeywordLists( const keywordPair_t* partialKeywordList, const keywordPair_t* fullKeywordList)
{// Returns true if the front of the full keyword lists is the same as the partial keyword list
	for(;;)
	{
		if( partialKeywordList->index != fullKeywordList->index
			|| strcasecmp( partialKeywordList->keyword, fullKeywordList->keyword) != 0)
		{// Any keyword or index mismatch and we're a mismatch
			return false;
		}
		partialKeywordList = partialKeywordList->iParent;
		fullKeywordList = fullKeywordList->iParent;
		if( partialKeywordList == nullptr)
		{// We've come to the end of our partial list, so we're good!
			return true;
		}
		if( fullKeywordList == nullptr)
		{// Ran out of full list with still more to go in the partial list.  This shouldn't happen,
		 // but it means we don't have a match.  Considered making this return true, as it would malke
		 // the two parameters symmetrical in behavior, but I think false is the correct semantic here.
			return false;
		}
	}
}

typedef struct InquiryContext
{// Used by InquireByKeyword to walk the tags looking for the value
	const keywordPair_t* keywordList;
	char** returnedParameters;
	char** returnedValue;
} InquiryContext_t;

static bool InquiryFinder( void* context, const keywordPair_t* keywordList, char* parameters, char* contentValue)
{//
	InquiryContext_t* ic = (InquiryContext_t *) context;
	if( CompareKeywordLists( ic->keywordList, keywordList))
	{// Got a match, return the value
		if( ic->returnedParameters)
			*(ic->returnedParameters) = parameters;
		if( ic->returnedValue)
			*(ic->returnedValue) = contentValue;
		return false;	// We're done
	}
	return true;	// Not found, keep looing
}


bool TinyXml::InquireByKeyword( const keywordPair_t* keywordList, char** returnedParameters, char** returnedValue)
{// Returns content for a specific nested list of keywords, true if found, false if not found
 // Because of the way the parser indexes non-unique keywords, the index is ignored if provided as 0,
 // but not ignored if provided non-zero.  This allows using this to index over tables.
 //
 // Tag heiarchy is only checked as deep as provided. Actual tag heiarchy may be deeper
 //
 // returnedValue is a direct pointer in the content, don't write back to it!
	if( iMyKeyword == nullptr)
 		return false;	// We are not initialized
	InquiryContext_t context = {keywordList, returnedParameters, returnedValue};
	return !IterateOverCcontent( &context, InquiryFinder, nullptr);
}

bool TinyXml::InquireByKeyword( char** returnedParameters, char** returnedValue, ...)	// Returns content for a specific nested list of keywords
{// Varargs version, note keywords go from outer to inner
 // After the firs two parameters, it's keyword followed by instance number for the tag nesting your are inquiring
 // Don't forget to add a trailing nullptr at the end of your parameter list or weird things will happen
 // Note you don't need all of the top levels.
	if( iMyKeyword == nullptr)
 		return false;	// We are not initialized
	va_list vl;
	va_start(vl,returnedValue);
	const keywordPair_t* nextParent = nullptr;
	keywordPair_t* query = nullptr;

	for(;;)
	{// Construct as keyword list
		char* nextKey = va_arg( vl, char*);
		if( nextKey == nullptr)
			break;
		int nextIndex = va_arg( vl, int);
		query = new keywordPair_t() ;
		query->iParent = nextParent;
		query->keyword = nextKey;
		query->index = nextIndex;
		nextParent = query;
	}
	va_end(vl);
	bool retval = InquireByKeyword( query, returnedParameters, returnedValue);
	const keywordPair_t* cquery = query;
	while( cquery)
	{
		nextParent = cquery->iParent;
		delete cquery;
		cquery = nextParent;
	}
	return retval;
}

bool TinyXml::IterateOverCcontent( void* context, ContentHandler_t handler, const keywordPair_t* parent)
{// Calls the handler for each terminus keyword. Stops iterating if hander returns false
	if( iMyKeyword == nullptr)
 		return false;	// We are not initialized
	keywordPair_t mykey;
	mykey.iParent = parent;
	mykey.keyword = iMyKeyword;
	mykey.index = iMyIndex;
	mykey.iSelfTerminated = iSelfTerminated;
	if( iXmlContent.size() == 0)
	{
		return (*handler)( context, &mykey, iMyParameters, iMyContent);
	}
	else
	{
		if(! (*handler)( context, &mykey, iMyParameters, nullptr))	// The null content indicates beginning
			return false;
		for( unsigned int i = 0; i < iXmlContent.size(); ++i)
		{
			if( !iXmlContent[i]->IterateOverCcontent( context, handler, &mykey))
				return false;	// Stop iterating when handler returns false
		}
		if(! (*handler)( context, &mykey, nullptr, nullptr))		// The null parameters indicates end
			return false;
	}
	return true;
}

#if 1
bool batoi( char *s, int *result)
{// A version of atoi that returns if it was successful or not
    char* done;
    errno = 0;
    s = SkipWhitespace( s);
    int64 checkval = strtoll( s, &done, 10);
    if( sizeof( int64) > sizeof( int))
    {
        if( (done == s) || (checkval > INT_MAX) || (checkval < INT_MIN) || (errno != 0))
            return false;
    }
    *result = (int) checkval;
    return true;
}

bool batoll( char *s, int64 *result)
{// A version of atoi that returns if it was successful or not
    char* done;
    errno = 0;
    s = SkipWhitespace( s);
    int64 checkval = strtoll( s, &done, 10);
    *result = checkval;
    return true;
}

char* Sanitize( char* pos)
{// Sanitize it by handling quotes and removing whitespace at both ends
	char* tail = SkipWhitespace( pos);
	if( *tail == '"')
	{
		pos = tail;
		tail = SkipQuotedString( pos);	// Returns pos at closing quote (or end of string)
		++pos;	// Skip opening quote
	}
	else
	{// Not quoted, skip printables skipping spaces, tabs, newlines, and other control characters
	 // The trick here is to allow spaces inside the string, but trailing spaces are removed
		pos = tail;
	 	tail = pos + strlen( pos);
		while( tail > pos)
		{// For all whitespace characters at the end
			--tail;
			if( *tail > ' ')
			{// We've found our last non-whitespace character, this is the end
				++tail;
				break;
			}
		}
	}
	// It is okay to zero out the tail character because if we are called a second time on the same string,
	// if it were quoted, it will take EndOfString as the closing quote. And, the whitespace is already gone.
	*tail = 0;
	return pos;
}


/*
		Unlike Sanitize, you can only call this once on a string.  If you call it a second time, any "&" characters will
		potentially behave badly.  Note, it does call Sanitize for you.
*/
char* ResolveQuotedSpecials( char* pos)
{
	char* retval = Sanitize( pos);
	char* outPos = pos;
	while( *pos)
	{// Copy the string handling special characters
		if( strncmp( pos, "&lt;", 4) == 0)
		{
			*outPos++ = '<';
			pos += 4;
		}
		if( strncmp( pos, "&gt;", 4) == 0)
		{
			*outPos++ = '>';
			pos += 4;
		}
		if( strncmp( pos, "&amp;", 5) == 0)
		{
			*outPos++ = '&';
			pos += 5;
		}
		else
			*outPos++ = *pos++;
	}
	return retval;
}

#endif


void TinyXml::SetPreserveCase( bool preserve)
{// Warning, this is currenlty a static setting
	sPreserveCase = preserve;
}
//     Output routines


XmlScopeSingleLine::XmlScopeSingleLine( const char* tag, const char* parameters)	// Scope of tag must be longer than this object
:
	myTag( tag),
	myXml( new std::ostringstream()),
	myIndentLevel( 0)
{// Only use this stream version of the constructor for the outermost nesting level
	*myXml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" XNL;
	if( parameters)
		*myXml << '<' << myTag << ' ' << parameters << '>';
	else
		*myXml << '<' << myTag << '>';
}

XmlScopeSingleLine::XmlScopeSingleLine( XmlScopeSingleLine &xml,
	const char* tag,			// Scope of tag must be longer than this object
	const char* parameters,		// Stuff before the ">"
	bool selfTermimate)			// Finsih with "/>"
:
	myTag( tag),
	myXml( xml.myXml),
	myIndentLevel( xml.myIndentLevel + 1)
{
	// Indent the tag for spaces for each indent level
	int localIndent = myIndentLevel * 4;
	while( --localIndent >= 0) *myXml<<' ';
	if( parameters)
		*myXml << '<' << myTag << ' ' << parameters;
	else
		*myXml << '<' << myTag;
	if( selfTermimate)
	{
		*myXml << "/>" << XNL;
		myTag = nullptr;	// Mark it as already closed
	}
	else
	{
		*myXml << '>';
	}
}

void XmlScopeSingleLine::CloseTag()
{// Can be used to close tag on the top level without closing the output stream
	if( myTag)
		*myXml << "</" << myTag << '>' << XNL;
	myTag = nullptr;
}

XmlScopeSingleLine::~XmlScopeSingleLine()
{// Emit the closing tag on the same line
	CloseTag();
	myTag = nullptr;
	if( myIndentLevel == 0)
	{//Deleting the top level tag, close out the whole stream
		delete myXml;
		myXml = nullptr;
	}
}

XmlScope::XmlScope( const char* tag, const char* parameters)
:
	XmlScopeSingleLine( tag, parameters)
{// Only use this stream version of the constructor for the outermost nesting level
	*myXml << XNL;
}

XmlScope::XmlScope( XmlScopeSingleLine &xml, const char* tag, const char* parameters)
:
	XmlScopeSingleLine( xml, tag, parameters, false)
{// The only thing we do different on construct is append a newline after the opening tag
	*myXml << XNL;
}

void XmlScope::CloseTag()
{// Can be used to close tag on the top level without closing the output stream
 // Indent the closing tag 4 spaces for each indent level prior to writing it out
	int localIndent = myIndentLevel * 4;
	while( --localIndent >= 0) *myXml<<' ';
	XmlScopeSingleLine::CloseTag();
}

XmlScope::~XmlScope()
{
	if( myTag) CloseTag();	//make sure we run OUR CloseTag
	myTag = nullptr;
}




// Used to indent-format the iXML, this group of XML helper routines is not thread safe, which may be an issue
// in the future if you attempt to multi-thread bin saves or the closing out of multiple channels in media files.

void XmlScopeSingleLine::WriteATagWithParams( const char* tag, const char* params, const char* value, size_t len)
{// Use this for simple tags that don't contain sub-tags, and optionally length limited
	XmlScopeSingleLine thisTag( *this, tag, params);

	while( *value && len--)
	{
		switch( *value)
		{
		case '<':
			s() << "&lt;";
			break;
		case '>':
			s() << "&gt;";
			break;
		case '&':
			s() << "&amp;";
			break;
		default:
			s() << *value;
			break;
		}
		++value;
	}
}
void XmlScopeSingleLine::WriteATag( const char* tag, const char* value, size_t len)
{
	WriteATagWithParams( tag, nullptr, value, len);
}


void XmlScopeSingleLine::PrintfATag( const char* tag, const char* format, ...)
{// Printf into an iXML block. Uses a 1K temp buffer, be nice to it or you will get an exception
	va_list p;
	va_start( p, format);
	char tempValue[ 1024] = {};
	int len = vsnprintf( tempValue, sizeof( tempValue), format, p);
	va_end( p);
	if( len >= sizeof( tempValue))
	{
		xraise( "iXML tag value longer than supported by PrintfATag", "str tag", tag, nullptr);
	}
	WriteATag( tag, tempValue);
}

void XmlScopeSingleLine::PrintfATagWithParams( const char* tag, const char* params, const char* format, ...)
{// Printf into an iXML block. Uses a 1K temp buffer, be nice to it or you will get an exception
	va_list p;
	va_start( p, format);
	char tempValue[ 1024] = {};
	int len = vsnprintf( tempValue, sizeof( tempValue), format, p);
	va_end( p);
	if( len >= sizeof( tempValue))
	{
		xraise( "iXML tag value longer than supported by PrintfATag", "str tag", tag, nullptr);
	}
	WriteATagWithParams( tag, params, tempValue);
}

struct WriteOutContext_t
{// Our keyword nesting on write
	XmlScope* iXs = nullptr;		// Scope to write our tags to
	WriteOutContext_t* iParentContext = nullptr;
	FILE* iOutFile = nullptr;		// Where the results go
};

bool WriteOutHandler( void* context, const keywordPair_t* keywordList, char* parameters, char* contentValue)
{// Used to output XML from an input XML object
	WriteOutContext_t* wc = (WriteOutContext_t*) context;

	if( parameters == nullptr)
	{// End of a keyword
		if( wc->iParentContext == nullptr)
		{// Topmost level, just close it, don't delete it, caller will do that
			wc->iXs->CloseTag();
		}
		else
		{// Not topmost, pop back a level
			delete wc->iXs;		// Close this keyword
			wc->iXs = wc->iParentContext->iXs;
			WriteOutContext_t* oldContext = wc->iParentContext;
			wc->iParentContext = oldContext->iParentContext;
			delete oldContext;
		}
	}
	else if( contentValue == nullptr)
	{// Start of a keyword
		if( wc->iXs == nullptr)
		{// This is the topmost level, create it on the topmost keyword
		 // Note we dont support paraemters at the topmost level
			wc->iXs = *parameters
				? new XmlScope( keywordList->keyword)//, parameters)	// This form of constructor doesn't exist. Yet.
				: new XmlScope( keywordList->keyword);
		}
		else
		{// Not topmost, push to next stack level
			WriteOutContext_t* oldContext = new WriteOutContext_t();
			oldContext->iXs = wc->iXs;		// Save old scope
			oldContext->iOutFile = wc->iOutFile;
			oldContext->iParentContext = wc->iParentContext;
			wc->iParentContext = oldContext;
			if( wc->iParentContext->iXs)
			{// Intermediate level
				wc->iXs = *parameters
					? new XmlScope( *wc->iParentContext->iXs, keywordList->keyword, parameters)
					: new XmlScope( *wc->iParentContext->iXs, keywordList->keyword);
			}
		}
	}
	else
	{// Terminus keyword
		bool hasMultipleLines = strchr( contentValue, '\n');
		if( hasMultipleLines)
		{// Multi-line cases
			if( parameters && *parameters)
			{// Parameters, construct with parameters
				XmlScope thisTag( *wc->iXs, keywordList->keyword, parameters);
				thisTag.s() << contentValue;
			}
			else
			{// No parameters, construct without, also omits the trailing space before the parameters
				XmlScope thisTag( *wc->iXs, keywordList->keyword);
				thisTag.s() << contentValue;
			}
		}
		else
		{// Single line cases
			if( parameters && *parameters)
			{// Parameters, construct with parameters
				XmlScopeSingleLine thisTag( *wc->iXs, keywordList->keyword, parameters, keywordList->iSelfTerminated);
				thisTag.s() << contentValue;
			}
			else
			{// No parameters, construct without, also omits the trailing space before the parameters
				XmlScopeSingleLine thisTag( *wc->iXs, keywordList->keyword);
				thisTag.s() << contentValue;
			}
		}
	}
	return true;
}

/*
		WriteOut emits the TinXml content to a file
*/
void WriteOut( TinyXml* inXml, FILE* outFile)
{// Writes out the xml to outFile as XML
	WriteOutContext_t context;
	inXml->IterateOverCcontent( &context, WriteOutHandler);
	context.iOutFile = outFile;
	fwrite( context.iXs->s().str().c_str(), 1, context.iXs->s().str().length(), outFile);
	delete context.iXs;
}
/*
		Helper for pulling key=value values from an XML tag's parameters
*/
char* PullValueFromParams( char* pos, const char* key)
{// Key should include '=' sign. Returns newly allocated string and transfers ownership
 // Note it isn't at all careful about aliasing keys in quoted strings. Just a hack that happens
 // to work as long as the key  with '=' sign doesn't appear elsewhere.
	if( pos == nullptr)
		return pos;
	pos = strstr( pos, key);
	if( pos == nullptr)
		return pos;
	pos += strlen( key);
	char quote = *pos;				// Remember the quote character
	char* startpos = ++pos;			// Skip the open quote
	while( *pos && *pos++ != quote);	// Skip to the close quote
	size_t len = (pos - startpos);
	char* retval = new char[  len];
	memcpy( retval, startpos, len);
	retval[ len-1] = 0;
	return retval;
}


void ReadCSVFile( char* fileContent, std::vector<std::vector<char*>> &csvContent)
{
	int row = 0;
	int col = 0;

	char* pos = fileContent;
	while(*pos)
	{
		csvContent.resize( row+1);
		csvContent[ row].resize( col+1);
		csvContent[ row][col] = pos;
		if( SkipEverythingUntil( pos, ',', "\n"))
		{// Terminated at newline, go to next row
			++row;
			col = 0;
		}
		else
		{// Termianted at comma, go to next column
			++col;
		}
	}
}

static char emptyString = 0;
char* TinyXml::InitializeFromJSON( char* pos, const char* parentKeyword)
{
// Recursively initialize a TinyXML thing.
 // BEWARE! the caracter string passed IS modified, terminating nuls are scattered througought,
 // and must have a lifetime that exceeds this object, as we keep pointers into it.  It is NOT
 // copied.
    if( !iMyKeyword)
    {
        iMyKeyword = "root";
        iMyParameters = &emptyString;
    }
	pos = SkipWhitespace( pos);		// BUG this allows XML comments
	if( *pos == 0)
	{// I guess we're done
	}
	else if( *pos == '[')
	{// Start list of items
		++pos;
		int index = 0;
		for(;;)
		{// add each item in the list
			if(*pos == ']')
			{
				++pos;
				break;		// End of list
			}
			iXmlContent.push_back( new TinyXml());
            iXmlContent[ index]->iMyIndex = index;
            iXmlContent[ index]->iMyKeyword = "Array-Element";
            iXmlContent[ index]->iMyParameters = &emptyString;
			pos = iXmlContent[ index]->InitializeFromJSON( pos, iMyKeyword);
			++index;
			pos = SkipWhitespace( pos);
			if(*pos == ',')
			{
				++pos;
			}
			else if(*pos == ']')
			{
				++pos;
				break;		// End of list
			}
			else
				xraise( "JSON parse failure, expected ',' or ']'", nullptr);
		}
	}
	else if( *pos == '{')
	{// I don't fully understand how '{' is different from '['
	 // other than '[' are unnamed.
		++pos;
        for( int index = 0;;++index)
		{// add each item in the list
			if(*pos == '}')
			{
				++pos;
				break;		// End of list
			}
			iXmlContent.push_back( new TinyXml());
			//iXmlContent[ index]->iMyIndex = index;
			pos = iXmlContent[ index]->InitializeFromJSON( pos, iMyKeyword);
			pos = SkipWhitespace( pos);
			if(*pos == ',')
			{
				*pos++ = 0;
			}
			else if(*pos == '}')
			{
				*pos++ = 0;
                Sanitize( iXmlContent[ index]->iMyContent);    // Temporary
				break;		// End of list
			}
			else
				xraise( "JSON parse failure, expected ',' or '}'", nullptr);
		}
	}
	else if( *pos == '"')
	{// Start of a keyword
		iMyKeyword = pos+1;
		pos = SkipQuotedString( pos);
		if( *pos)
		{// Not done, Terminate the keyword and get contents
            iMyParameters = pos;
			*pos++ = 0;
			pos = SkipWhitespace( pos);
			if( *pos != ':')
				xraise( "JSON parse failure, expected ':'", nullptr);
			pos = SkipWhitespace( pos+1);
			iMyContent = pos;
			if( *iMyContent == '{' ||  *iMyContent == '[')
			{// Content is a list, recurse to initialize the list
				pos = InitializeFromJSON( pos, parentKeyword);
			}
			else
			{// Content from here to comma
				pos = SkipEverythingUntilButLeaveBehind( pos, ',', '}');
			}
		}
	}
	else
		xraise( "JSON parse failure, expected '{', '[', or a quoted keyword", nullptr);
	return pos;
}
