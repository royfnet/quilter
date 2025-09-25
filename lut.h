#ifndef _LUT_H
#define _LUT_H

/*		lut.h		Lookup table template class

		Used to create classes of slow-insert fast-lookup tables.
		
		To be used in a table, you must have a method "char* sname( int *len)" that returns
		the address of your name and its length.  This should be fast, preferably inline.
		Don't worry about returning a shallow-copy address.  There should also be a
		"char* dname( int *len, int index)" which returns a displable string for the user.
*/

#define CKPTR(p) //do nothing

//		If something unexpecte happens, "throw up" will signal an exception that reports
//		the error source file name and line number.
const char* sourcelocation( const char* , int);
#define up sourcelocation(__FILE__, __LINE__)

/*
		If you only need management of symbol names, and not their corresponding values,
		(such as, creating a list box or menu from a symbol table), you can operate on the
		"abstractlut" class, and not have to know what kinds of things the list you are
		manipulating contains.
		
		However, your operations are limited to various kinds of name lookups. They operate
		on numeric indicies in order to simplify mapping to menu item and list box selections.
*/
class abstractlut		// Abstract symbol table - don't know what type
{// Abstract class
public:
	virtual ~abstractlut() {}

	virtual int count(		// Returns number of symbol name matches
		const char* symbol,		// Symbol to match (front string) (may be NULL)
		int length,				// Length of symbol 
		int *place)=0;			// Returned index of first match
		
	virtual const char* nameptr(	// Wraps a call to the "thing"'s sname() routine
		int *len,				// Length of the name returned
		int index)=0;			// Index, must be between count() and count()+place

	const char* namecpy(	// Returns a NULL-terminated copy of a name.
		int index,				// Index, must be between count() and count()+place
		char* buf=0,			// Buffer to put name into
		int blen=128);			// Size of buffer

	virtual const char* displayptr(// Wraps a call to the "thing"'s dname() routine
		int *len,				// Length of the name returned
		int index)=0;			// Index, must be between count() and count()+place

	const char* displaycpy(		//	Returns a NULL-terminated copy of the display name
		int index,				// Index, must be between count() and count()+place
		char* buf=0,			// Buffer to put name into
		int blen=128);			// Size of buffer

protected:
    int members;		// Number of things in the table
    abstractlut()			// Construct with zero members
    :
		members( 0)
	{}
};

template< class THING> class lut : public abstractlut
{
public:
	lut( void);					// Constructor 
	~lut( void);

	int add(					// Add an item to the table, 1 is success
		THING *value);			// Item to add

	int remove( 				// Returns result code, 1 is success
		THING *value);			// Item to remove

	THING *lookup_a(			// Returns first matching member, or NULL if none 
		const char* symbol,		// Front string to match
		int length,				// Symbol length
		int &count);			// Count of ambiguous partial matches, or characters

	THING *lookup(				// Returns matching member, requires exact match
		const char* symbol,		// Front string to match
		int length);			// Symbol length

	THING *lookup_a(			// Returns first matching member, or NULL if none
		const char* symbol,		// Front string to match
		int &count);			// Count of ambiguous partial matches, or characters

	THING *lookup(				// Returns matching member, requires exact match
		const char* symbol);		// Front string to match

	THING *first( void)			// Returns first item in list, or NULL if empty
		{ return( table ? table[ 0] : 0);}	// Same as getambiguous( NULL, 0)
/*
		For looping on members - loops are delete-safe - you can add and delete
		while on a member.  Note that adds may or may not be included in the current
		iteration.
*/
	THING *next( THING *current);

	void selftest( void);			// Test instance for validity [out of order, etc.]

	virtual int count(		// Returns number of symbol name matches
		const char* symbol,		// Symbol to match (front string) (may be NULL)
		int length,				// Length of symbol 
		int *place);			// Returned index of first match (may be NULL)
		
	virtual const char* nameptr(	// Wraps a call to the "thing"'s sname() routine
		int *len,				// Length of the name returned
		int index);				// Index, must be between count() and count()+place

	virtual const char* displayptr(	// Wraps a call to the "thing"'s dname() routine
		int *len,				// Length of the name returned
		int index);				// Index, must be between count() and count()+place

private:
    THING **table;
    int allocated;
/*
		Normally locate is private, but location information is useful to
		menu items and other places where the list is visible to the user. They
		can use count().
*/	
	int locate(				// Find position of symbol in table - returns 0 if found 
		const char* symbol,		// Symbol to match
		int length,				// Length of symbol 
		int *place);			// Returned index of item, or location to insert if not found
};

/*
		Some useful global string management routines, required for thing class
*/
int lutcmp(		// strcmp for counted strings 
	const char* s1, int n1,
	const char* s2, int n2);

int lutncmp(		// strncmp for counted strings 
	const char* s1, int n,
	const char* s2, int n2);

int circmp(	   	/* Compare string to circular buffer */
	const char* s1,	/* Circular buffer */
	int l1,		/* Size of buffer */
	int n1,		/* Insertion point (one past end) */
	const char* s2,	/* String to compare to */
	int n2);	/* Length of string to compare to, MUST BE <= l1 */

void lutclasstest( void);	// Regression test this class templace

template< class THING> lut<THING>::lut( void)	// Construct table
:
	table( 0),
	allocated( 0)
{//	Note we don't allocate table here, so that you can have static global symbol tables.
}

template< class THING> lut<THING>::~lut( void)	// Destructor, note it doesn't delete contents
{				   
	if( table) delete[] table;
	table = NULL;
}

template< class THING> int lut<THING>::locate(	// Locate position of entry by symbol name
	const char* symbol,				// Symbol to locate, any string
	int length,					// Symbol length
	int *place)					// Returned index
/*
        Locates the index in the table where a symbol is, or should go.
        Returns 0 if exact match, non-0 otherwise.
*/
{
    int top = 0;
	int bottom = members;   // Bounds of binary search
    int i = 0;
	int s = 1;	// Loop index and compare result
    int n = length & 0xFF;		// Length if input symbol
    const char* isymb;			// Symbol name of item under test
    int ilen;					// Length of item symbol

    for(;;)
    { /* For each iteration of binary search */
        if( bottom == top) break;       // Not found, or list empty
        i = ((bottom-top)>>1) + top;
        isymb = table[ i]->sname( &ilen);
        s = lutcmp( symbol, n, isymb, ilen);
        if( s == 0) break;              // Found (s == 0)
        else if( s < 0)
        { // Target is in bottom half
            if( top == i) break;        // Not found, insert at i
            bottom = i;         		// Current is new lower bound
        }
        else
        { // Target is in top half
            if( top == i) {i++; break;} // Not found, insert after i
            top = i;    				// Current is new upper bound
		}
    }
    if( place)
		*place = i;
    return( s);
}

template < class THING> int lut<THING>::count(		// Returns number of symbol name matches
	const char* symbol,		// Symbol to match (front string) (may be NULL)
	int length,				// Length of symbol 
	int *place)				// Returned index of first match (may be NULL)
{
	int c;
	THING *ret = lookup_a( symbol, length, c);

	locate( symbol, length, place);	// Compute the place
	if( ret == NULL) return( 0);	// None matched (don't return c as length needed to match)
	return( c);						// Return count of matches
}

template < class THING> const char* lut<THING>::nameptr(	// Wraps a call to the "thing"'s sname() routine
	int *len,				// Length of the name returned
	int index)				// Index, must be between count() and count()+place
{
	CKPTR( table);
	if( (index < 0) || (index >= members)) throw up;
	return( table[ index]->sname( len));
}

template < class THING> const char* lut<THING>::displayptr(	// Wraps a call to the "thing"'s dname() routine
	int *len,				// Length of the name returned
	int index)				// Index, must be between count() and count()+place
{
	CKPTR( table);
	if( (index < 0) || (index >= members)) throw up;
	return( table[ index]->dname( len, index));
}

template < class THING> int lut<THING>::add( // Define a symbol
	THING *value)		// user's symbol and value */
/*
        Returns 1 if successful, and 0 (FALSE) if this would be a duplicate
        symbol.
*/
{
	int length;									// Length of symbol
	const char* symbol = value->sname( &length);
	int i;

	if( !locate( symbol, length, &i)) return(0);	// Found!  Fail the add
/*
		Symbol not found, move the rest of the table down to make room for it
*/
	if( allocated <= members)
	{
		allocated += 64;			// Increase allocation
		THING **xtable = new THING*[ allocated];			// Get a new table
		if( table)
		{// The old table exists, move value from it
			memcpy( xtable, table, members*sizeof(*table));	// Copy into new table
			delete[] table;				// Remove old table
		}
		table = xtable;				// Take to using new table
	}
	for( int j( members); j>i; j--)
		table[ j] = table[ j-1];
	members = members + 1;			// Mark the extra space taken
	table[ i] = value;				// Insert this item
	return( 1);
}

template< class THING> int lut<THING>::remove( // Returns result code, 1 is success
	THING *value)							// Item to remove
{
/*
		Removes a symbol from the table.  Returns non-zero if the symbol was not
		found. Note you must give the exact object.  If you don't have it, get it
		with lookup()!
*/
	int i, s;
	int plen;
	const char* pname = value->sname( &plen);
	
	s = locate( pname, plen, &i);
	if( s != 0) return( 0);				// Must have found it
	if( table[ i] != value) return( 0);	// Must be the same object
/*
		Symbol was found, delete it by moving everything up
*/
	members--;
	for( int j = i; j < members; j++)
		table[ j] = table[ j+1];
	table[ members] = 0;	// Make sure unused entries are NULL
	if( members == 0)
	{// Once all members of the table are deleted, delete the table itself.
	 // This allows us to have static global tables that don't leave memory behind
	 // when the program exits, so int as you are sure you remove everything from the table
	 	allocated = 0;
	 	delete[] table;
	 	table = NULL;
	}
	return( 1);
}

template< class THING> THING *lut<THING>::next(
	THING *current)				// Current item (start from first()) from which to get next
{ // Get's next symbol in table.  Returns NULL if done iterating
	const char* cname;			// Name of current
	int clen;					// Length of current
	int i;						// Where we are
	int s;						// Status of find
	
	cname = current->sname( &clen);		// Find name of where we are now
	s = locate( cname, clen, &i);		// Find out where we are    
    if( s == 0) i++;					// Exact match is this one, get the next one
    if( i >= members) return( NULL);	// We're off the end
	else return( table[ i]);    		// Or we're not
}

template< class THING> THING *lut<THING>::lookup_a( // lookup a symbol, allow ambiguous
	const char* symbol,		// Symbol to look up
	int length,			// Length of symbol
	int &countParam)	// Returned count of matches, or match length
{
/*
		Returns the list of symbols whose front string matches the given symbol.
		If at least one matches, count is returned as the number which matched.
		If none match, the return value is NULL, and count is returned as the
		length to which you must shorten the symbol to get something to match.
*/
    int i;
	int place;
	
    if( length == 0)
    { /* User wants entire list, Handle this as a fast special case */
        countParam = members;
        return( table ? table[ 0] : 0);
    }
    i = locate( symbol, length, &place);
    if( i == 0)
    { /* Exact match special case, force count to one even if ambiguous */
    	countParam = 1;
    	return( table[ place]);
    }
/*
        Compute the number of partial matches.
*/
	for( i=0; place+i < members; i++)
	{ // For each partial match
		int ilen;									 // Matching symbol len
		const char* isymb = table[place+i]->sname( &ilen);
		
		if( lutncmp( symbol, length, isymb, ilen) != 0)	// Break if not a partial match
			break;
	}
	if( i == 0)
	{ // No matches, figure out longest string which matches
	    i = 0;
	    countParam = 0;
/*
        Run "i" for as int as the symbol we were given, and the symbol
        following us alphabetically are equal.  Then back up one symbol, and
        count the match length for that one.  Take whichever is longer.
        
        Note that in both cases, we don't need to check for reaching the end
        of our symbol, because that would mean there was a front match (unique
        or ambiguous) and we already know that didn't happen.
*/
        if( place < members)
        { // There is a symbol in the table to count
        	int ilen;									// Following symbol len
			const char* isymb = table[place]->sname( &ilen);

            for(; symbol[i] == isymb[i]; i++)
            	if( i == ilen) break;
        }
/*
        Try the one alphabetically before it
*/
		if( place > 0) place--;     /* Start before it alphabetically */
		if( place < members)
		{ // There is still a symbol here to look at
        	int ilen;									// Preceeding symbol len
			const char* isymb = table[place]->sname( &ilen);

			for( countParam = 0; symbol[ countParam] == isymb[ countParam]; countParam++)
                if( countParam == ilen) break;
        }
/*
        Use whichever one is longer
*/
        if( countParam < i) countParam = i;
		return( NULL);
	}
	countParam = i;
	return( table[ place]);
}

template< class THING> THING *lut<THING>::lookup(	// Returns matching member, requires exact match
	const char* symbol,		// Front string to match
	int length)				// Symbol length
{
	int c;
	THING *ret = lookup_a( symbol, length, c);
	if( ret == NULL) return( NULL);	// Disallow non-matches
	if( c != 1) return( NULL);		// Disallow ambiguious matches
	ret->sname( &c);					// Get the length of the presumed matching symbol
	if( length != c) return( NULL);	// Disallow unique abbreviations - must be exact match
	return( ret);
}

template< class THING> THING *lut<THING>::lookup(	// Returns matching member, requires exact match
	const char* symbol)		// Exact string to match
{
	return lookup( symbol, (int) strlen( symbol));
}

template< class THING> THING *lut<THING>::lookup_a(	// Returns matching member, requires exact match
	const char* symbol,		// Unambiguous match
	int &countParam)		// Returned count of matches, or match length
{
	return lookup_a( symbol, strlen( symbol), countParam);
}

template< class THING> void lut<THING>::selftest( void)	// test an object
{
}

/*	End of lut.h */
#endif
