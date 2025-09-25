/*		JeffSema.c	
*/

#include "JeffSema.h"
#include <errno.h>

JeffSemaphore::JeffSemaphore()
:
	iCurrentCount( 0)
{
	pthread_mutex_init( &iMutex, nullptr);
	pthread_cond_init( &iCond, nullptr);
}

bool JeffSemaphore::Decrement( const struct timespec * wakeupTime)
{// Should probably return something if we timed out with
	bool retval = true;
	pthread_mutex_lock( &iMutex);
	while( iCurrentCount == 0)
	{// Until the mutex is released, wait for it
		if( ETIMEDOUT == pthread_cond_timedwait( &iCond, &iMutex, wakeupTime))
		{
			++iCurrentCount;	// Make up for decrement after break
			retval = false;
			break;
		}
	}
	--iCurrentCount;
	pthread_mutex_unlock( &iMutex);
	return retval;
}

bool JeffSemaphore::Decrement( )
{
	pthread_mutex_lock( &iMutex);
	while( iCurrentCount == 0)
	{// Until the mutex is released, wait for it
		pthread_cond_wait( &iCond, &iMutex);
	}
	iCurrentCount--;
	pthread_mutex_unlock( &iMutex);
	return true;
}

bool JeffSemaphore::Increment(
	unsigned long count)			// How far to increment
{
	bool retval = true;
	pthread_mutex_lock( &iMutex);
	iCurrentCount += count;
	if( iCurrentCount == 1)
		pthread_cond_signal( &iCond);
	else
		pthread_cond_broadcast( &iCond);
	pthread_mutex_unlock( &iMutex);
	return retval;
}

JeffSemaphore::~JeffSemaphore()
{
	pthread_cond_destroy( &iCond);
	pthread_mutex_destroy( &iMutex);
}

