/*		JeffSema.h		Semaphore defines
*/
#ifndef JeffSema_h
#define JeffSema_h

#define _TIMESPEC_DEFINED   // pthread.h needs this on Windows to avoid duplicate definition
#include <pthread.h>

class JeffSemaphore
{
private:
	unsigned long iCurrentCount;		// Current state, 0 is unlocked
	pthread_mutex_t iMutex;				// Locks semaphore's counter and condition
	pthread_cond_t iCond;				// Conditions the value of the semaphore

public:
	JeffSemaphore();
	
	~JeffSemaphore();
	
	bool Decrement(	);			// Decrement (should include a wait time)

	bool Decrement( const struct timespec * wakeupTime);

	bool Increment(				// Increment (signal)
		unsigned long count = 1);			// How many times to increment count
	
	inline unsigned long Count() const	// Returns current count
		{return iCurrentCount;}
};


/*	End of JeffSema.h */
#endif
