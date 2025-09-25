//
//  TinyXML.hpp
//  audio
//
//  Created by Jeffrey Lomicka on 7/7/18.
//  Copyright © 2018 Jeffrey Lomicka. All rights reserved.
//

#ifndef TinyXML_hpp
#define TinyXML_hpp
#include <vector>
#include <sstream>

typedef struct keywordPair
{// Lists of these are used to define where you are
	const struct keywordPair* iParent = nullptr;	// Nesting
	const char* keyword = nullptr;	// Keyword you are in
	int index=0;					// Instance of this keyword in the parent
	bool iSelfTerminated = false;	// Keyword was termnianted with "/>"
} keywordPair_t;

typedef bool (*ContentHandler_t)( void* context, const keywordPair_t* keywordList, char* parameters, char* contentValue);

class TinyXml
{// Works on a buffer of XML, which it will destory in the process of parsing and dispatching
public:
	virtual ~TinyXml();
	char* Initialize( char* pos, const char* parentKeyword = nullptr);
	char* InitializeFromJSON( char* pos, const char* parentKeyword = nullptr);
	bool InquireByKeyword( const keywordPair_t* keywordList, char** returnedParameters, char** returnedValue);	// Returns content for a specific nested list of keywords
	bool InquireByKeyword( char** returnedParameters, char** returnedValue, ...);	// Returns content for a specific nested list of keywords. index pairs
	bool IterateOverCcontent( void* context, ContentHandler_t handler, const keywordPair_t* parent = nullptr);
	void SetPreserveCase( bool preserve);
private:
	const char* iMyKeyword = nullptr;			// This is our keyword
	char* iMyParameters = nullptr;		// This all the stuff between the keyword and the '>', which is enitrely the client's problem
	char* iMyContent = nullptr;			// This is our content, may or may not be more XML tags
	int iMyIndex = 0;					// This is our index for this keyword within our parent
	bool iSelfTerminated=false;			// True if this came from a <keyword /> construct
	std::vector<TinyXml*> iXmlContent;	// If content is just more XML tags
};
char* ResolveQuotedSpecials( char* pos);
char* Sanitize( char* pos);
/*
		Utility routines for generating iXML metadata
*/

#define XNL "\r\n"	// Newline sequence for XML output.

class XmlScopeSingleLine
{// Emits both TAG and /TAG across the scope of the object, entire object can be one line
public:
	const char* myTag;
	std::ostringstream *myXml;
	int myIndentLevel;

	XmlScopeSingleLine( const char* tag, const char* parameters = nullptr);	// Scope of tag must be longer than this object
	XmlScopeSingleLine( XmlScopeSingleLine &xml, const char* tag, const char* parameters=nullptr, bool selfTermimate=false);	// Scope of tag must be longer than this object
	virtual void CloseTag();
	virtual ~XmlScopeSingleLine();
	void WriteATag( const char* tag, const char* value, size_t len = INT32_MAX);
	void PrintfATag( const char* tag, const char* format, ...);
	void WriteATagWithParams( const char* tag, const char* parameters, const char* value, size_t len = INT32_MAX);
	void PrintfATagWithParams( const char* tag, const char* parameters, const char* format, ...);
	inline std::ostringstream& s() const
	{// Access to the stream for content
		return *myXml;
	}
};

class XmlScope : public XmlScopeSingleLine
{// Emits both TAG and /TAG across the scope of the object, tags on lines by themselves
public:
	XmlScope( const char* tag, const char* parameters = nullptr);
	XmlScope( XmlScopeSingleLine &xml, const char* tag, const char* parameters = nullptr);
	virtual void CloseTag();
	virtual ~XmlScope();
};

/*
		Writes out the TineyXML structure to a file
*/
void WriteOut( TinyXml* inXml, FILE* outFile);

/*
		Helper for pulling key=value values out of a tag's parameter list
		Slightly imperfect, doesn't handle the case where an alias to the key is protected by quotes,
		also doesn't handle keys that are tail strings to other keys,  Ripe for being rewritten.
*/
char* PullValueFromParams( char* pos, const char* key);

/*
		Slightly imperfect CSV reader, good enough for our stuff
*/
void ReadCSVFile( char* fileContent, std::vector<std::vector<char*>> &csvContent);

#endif /* TinyXML_hpp */
