/* cds_tprim.h -- portable threading/concurrency primitives in C89
 * Do this:
 *   #define CDS_TPRIM_IMPLEMENTATION
 * before including this file in *one* C/C++ file to provide the function implementations.
 *
 * For a unit test:
 *   cc -pthread -x c -DCDS_TPRIM_TEST -o [exeFile] cds_tprim.h
 */
#if !defined(CDS_TPRIM_H)
#define CDS_TPRIM_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef CDS_TPRIM_STATIC
#	define CDS_TPRIM_DEF static
#else
#	define CDS_TPRIM_DEF extern
#endif
	
#ifdef _MSC_VER
#	include <windows.h>
#	define CDS_TPRIM_INLINE __forceinline
#else
#   include <pthread.h>
#	include <semaphore.h>
#	ifdef __cplusplus
#		define CDS_TPRIM_INLINE inline
#	else
#		define CDS_TPRIM_INLINE
#	endif
#endif

#if defined(_MSC_VER)
typedef struct
{
	HANDLE mHandle;
	LONG mCount;
} cds_tprim_fastsem_t;
#else
typedef struct
{
	sem_t mSem;
	int mCount;
} cds_tprim_fastsem_t;
#endif

/** @brief Initialize a semaphore to the specified value. */
CDS_TPRIM_DEF int cds_tprim_fastsem_init(cds_tprim_fastsem_t *sem, int n);

/** @brief Destroy an existing semaphore object. */
CDS_TPRIM_DEF void cds_tprim_fastsem_destroy(cds_tprim_fastsem_t *sem);

/** @brief Decrement the semaphore's internal counter. If the counter is <=0, this
 *         call will block until it is positive again before decrementing.
 *  @return 0 if the semaphore was initialized successfully; non-zero if an
 *          error occurred.
 */
CDS_TPRIM_DEF void cds_tprim_fastsem_wait(cds_tprim_fastsem_t *sem);

/** @brief Decrement the semaphore's internal counter. Unlike sem_wait(), this
 *         variant returns immediately if the count is <=0 instead of blocking.
 *  @return 1 if the count was decremented successfully; 0 if not.
 */
CDS_TPRIM_DEF int cds_tprim_fastsem_trywait(cds_tprim_fastsem_t *sem);

/** @brief Increment the semaphore's internal counter. If the counter is <=0,
 *         this call will wake a thread that had previously called sem_wait().
 */
CDS_TPRIM_DEF void cds_tprim_fastsem_post(cds_tprim_fastsem_t *sem);

/** @brief Functionally equivalent to calling sem_post() n times. This variant
 *         may be more efficient if the underlying OS semaphore allows multiple
 *         threads to be awakened with a single kernel call.
 */
CDS_TPRIM_DEF void cds_tprim_fastsem_postn(cds_tprim_fastsem_t *sem, int n);

/** @brief Retrieves the current value of the semaphore's internal counter,
 *         for debugging purposes. If the counter is positive, it represents
 *         the number of available resources that have been posted. If it's
 *         negative, it is the (negated) number of threads waiting for a
 *         resource to be posted.
 *  @return The current value of the semaphore's internal counter.
 */
CDS_TPRIM_DEF int cds_tprim_fastsem_getvalue(cds_tprim_fastsem_t *sem);

#if defined(_MSC_VER)
typedef struct
{
	CONDITION_VARIABLE mCond;
	/* TODO */
} cds_tprim_eventcount_t;
#else
typedef struct
{
	pthread_cond_t mCond;
	pthread_mutex_t mMtx;
	int mCount;
} cds_tprim_eventcount_t;
#endif

/** @brief Initialize an eventcount object. */
CDS_TPRIM_DEF int cds_tprim_eventcount_init(cds_tprim_eventcount_t *ec);

/** @brief Destroy an eventcount object. */
CDS_TPRIM_DEF void cds_tprim_eventcount_destroy(cds_tprim_eventcount_t *ec);

/** @brief Return the current event count, in preparation for a call to wait().
 *         Often called prepare_wait().
 *  @note This is *not* a simple query function; the internal object state is modified.
 */
CDS_TPRIM_DEF int cds_tprim_eventcount_get(cds_tprim_eventcount_t *ec);

/** @brief Wakes all threads waiting on an eventcount object, and increment the internal counter.
 *         If no waiters are registered, this function is a no-op.
 */
CDS_TPRIM_DEF void cds_tprim_eventcount_signal(cds_tprim_eventcount_t *ec);

/** @brief Compare the provided count to the eventcount object's internal counter; if they match,
 *         put the calling thread to sleep until the eventcount is signalled.
 *         If the counts do not match, the caller returns more-or-less immediately.
 */
CDS_TPRIM_DEF void cds_tprim_eventcount_wait(cds_tprim_eventcount_t *ec, int cmp);

#if defined (_MSC_VER)
typedef struct
{
	
} cds_tprim_monsem_t;
#else
typedef struct
{
	int mState;
	cds_tprim_eventcount_t mEc;
	cds_tprim_fastsem_t mSem;
} cds_tprim_monsem_t;
#endif

CDS_TPRIM_DEF int  cds_tprim_monsem_init(cds_tprim_monsem_t *ms, int count);
CDS_TPRIM_DEF void cds_tprim_monsem_destroy(cds_tprim_monsem_t *ms);
CDS_TPRIM_DEF void cds_tprim_monsem_wait_for_waiters(cds_tprim_monsem_t *ms, int waitForCount);
CDS_TPRIM_DEF void cds_tprim_monsem_wait_no_spin(cds_tprim_monsem_t *ms);
CDS_TPRIM_DEF int  cds_tprim_monsem_try_wait(cds_tprim_monsem_t *ms);
CDS_TPRIM_DEF int  cds_tprim_monsem_try_wait_all(cds_tprim_monsem_t *ms);
CDS_TPRIM_DEF void cds_tprim_monsem_wait(cds_tprim_monsem_t *ms);
CDS_TPRIM_DEF void cds_tprim_monsem_post(cds_tprim_monsem_t *ms);
CDS_TPRIM_DEF void cds_tprim_monsem_postn(cds_tprim_monsem_t *ms, int n);


#ifdef __cplusplus
}
#endif

#endif /*-------------- end header file ------------------------ */

#if defined(CDS_TPRIM_TEST)
#	if !defined(CDS_TPRIM_IMPLEMENTATION)
#		define CDS_TPRIM_IMPLEMENTATION
#	endif
#endif

#define CDS_TPRIM_IMPLEMENTATION // TEMP
#ifdef CDS_TPRIM_IMPLEMENTATION

#include <assert.h>
#include <limits.h>

#define CDS_TPRIM_MIN(a,b) ( (a)<(b) ? (a) : (b) )

int cds_tprim_fastsem_init(cds_tprim_fastsem_t *sem, int n)
{
#if defined(_MSC_VER)
	sem->mCount = n;
	sem->mHandle = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
	return 0; /* TODO: check return value of CreateSemaphore() */
#else
	sem->mCount = n;
	int err = sem_init(&sem->mSem, 0, 0);
	return err;
#endif
}

void cds_tprim_fastsem_destroy(cds_tprim_fastsem_t *sem)
{
#if defined(_MSC_VER)
	CloseHandle(sem->mHandle);
#else
	sem_destroy(&sem->mSem);
#endif
}

int cds_tprim_fastsem_trywait(cds_tprim_fastsem_t *sem)
{
#if defined(_MSC_VER)
	LONG count = InterlockedExchangeAdd(&sem->mCount, 0);
	while(count > 0)
	{
		if (InterlockedCompareExchange(&sem->mCount, count-1, count) == count)
		{
			return 1;
		}
		/* otherwise, count was reloaded. backoff here is optional. */
	}
	return 0;
#else
	int count = __sync_fetch_and_add(&sem->mCount, 0);
	while(count > 0)
	{
		int newCount = count-1;
		count = __sync_val_compare_and_swap(&sem->mCount, count, newCount);
		if (count == newCount)
		{
			return 1;
		}
		/* otherwise, count was reloaded. backoff here is optional. */		
	}
	return 0;
#endif
}

static void cds_tprim_fastsem_wait_no_spin(cds_tprim_fastsem_t *sem)
{
#if defined(_MSC_VER)
	if (InterlockedExchangeAdd(&sem->mCount, -1) < 1)
	{
		WaitForSingleObject(sem->mHandle, INFINITE);
	}
#else
	if (__sync_fetch_and_add(&sem->mCount, -1) < 1)
	{
		sem_wait(&sem->mSem);
	}
#endif
}

void cds_tprim_fastsem_wait(cds_tprim_fastsem_t *sem)
{
#if defined(_MSC_VER)
	int spin_count = 1;
	while(spin_count--)
	{
		if (cds_tprim_fastsem_trywait(sem) )
		{
			return;
		}
	}
	cds_tprim_fastsem_wait_no_spin(sem);
#else
	int spin_count = 1;
	while(spin_count--)
	{
		if (cds_tprim_fastsem_trywait(sem) )
		{
			return;
		}
	}
	cds_tprim_fastsem_wait_no_spin(sem);
#endif
}

void cds_tprim_fastsem_post(cds_tprim_fastsem_t *sem)
{
#if defined(_MSC_VER)
	LONG oldCount = InterlockedExchangeAdd(&sem->mCount, 1);
	if (oldCount < 0)
	{
		ReleaseSemaphore(sem->mHandle, 1, 0);
	}
#else
	int oldCount = __sync_fetch_and_add(&sem->mCount, 1);
	if (oldCount < 0)
	{
		sem_post(&sem->mSem);
	}
#endif
}

void cds_tprim_fastsem_postn(cds_tprim_fastsem_t *sem, int n)
{
#if defined(_MSC_VER)
	LONG oldCount = InterlockedExchangeAdd(&sem->mCount, n);
	if (oldCount < 0)
	{
		int numWaiters = -oldCount;
		int numToWake = CDS_TPRIM_MIN(numWaiters, n);
		ReleaseSemaphore(sem->mHandle, numToWake, 0);
	}
#else
	int oldCount = __sync_fetch_and_add(&sem->mCount, n);
	if (oldCount < 0)
	{
		int numWaiters = -oldCount;
		int numToWake = CDS_TPRIM_MIN(numWaiters, n);
		/* wakeN would be better than a loop here, but hey */
		int iWake;
		for(iWake=0; iWake<numToWake; iWake += 1)
		{
			sem_post(&sem->mSem);
		}
	}
#endif
}

int cds_tprim_fastsem_getvalue(cds_tprim_fastsem_t *sem)
{
#if defined(_MSC_VER)
    return (int)InterlockedExchangeAdd(&sem->mCount, 0);
#else
	return __sync_fetch_and_add(&sem->mCount, 0);
#endif
}


int cds_tprim_eventcount_init(cds_tprim_eventcount_t *ec)
{
#if defined(_MSC_VER)
#else
	pthread_cond_init(&ec->mCond, 0);
	pthread_mutex_init(&ec->mMtx, 0);
	__sync_fetch_and_and(&ec->mCount, 0); /* mCount=0 */
	return 0;
#endif	
}
void cds_tprim_eventcount_destroy(cds_tprim_eventcount_t *ec)
{
#if defined(_MSC_VER)
#else
	pthread_cond_destroy(&ec->mCond);
	pthread_mutex_destroy(&ec->mMtx);
#endif	
}
int cds_tprim_eventcount_get(cds_tprim_eventcount_t *ec)
{
#if defined(_MSC_VER)
#else
	return __sync_fetch_and_or(&ec->mCount, 1);
#endif	
}
void cds_tprim_eventcount_signal(cds_tprim_eventcount_t *ec)
{
#if defined(_MSC_VER)
#else
	int key = __sync_fetch_and_add(&ec->mCount, 0);
	if (key & 1)
	{
		int newKey = (key+2) & ~1;
		pthread_mutex_lock(&ec->mMtx);
		key = __sync_val_compare_and_swap(&ec->mCount, key, newKey);
		while (key != newKey)
		{
			/* spin */
			newKey = (key+2) & ~1;
			key = __sync_val_compare_and_swap(&ec->mCount, key, newKey);			
		}
		pthread_mutex_unlock(&ec->mMtx);
		pthread_cond_broadcast(&ec->mCond);
	}
#endif	
}
void cds_tprim_eventcount_wait(cds_tprim_eventcount_t *ec, int cmp)
{
#if defined(_MSC_VER)
#else
	pthread_mutex_lock(&ec->mMtx);
	if ((__sync_fetch_and_add(&ec->mCount, 0) & ~1) == (cmp & -1))
	{
		pthread_cond_wait(&ec->mCond, &ec->mMtx);
	}
	pthread_mutex_unlock(&ec->mMtx);
#endif
}

#define CDS_TPRIM_MONSEM_COUNT_SHIFT 8
#define CDS_TPRIM_MONSEM_COUNT_MASK  0xFFFFFF00UL
#define CDS_TPRIM_MONSEM_COUNT_MAX   ((unsigned int)(CDS_TPRIM_MONSEM_COUNT_MASK) >> (CDS_TPRIM_MONSEM_COUNT_SHIFT))
#define CDS_TPRIM_MONSEM_WAIT_FOR_SHIFT 0
#define CDS_TPRIM_MONSEM_WAIT_FOR_MASK 0xFF
#define CDS_TPRIM_MONSEM_WAIT_FOR_MAX ((CDS_TPRIM_MONSEM_WAIT_FOR_MASK) >> (CDS_TPRIM_MONSEM_WAIT_FOR_SHIFT))

int  cds_tprim_monsem_init(cds_tprim_monsem_t *ms, int count)
{
	assert(count >= 0);
#if defined(_MSC_VER)
#else
	cds_tprim_fastsem_init(&ms->mSem, 0);
	cds_tprim_eventcount_init(&ms->mEc);
	ms->mState = count << CDS_TPRIM_MONSEM_COUNT_SHIFT;
	return 0;
#endif
}
void cds_tprim_monsem_destroy(cds_tprim_monsem_t *ms)
{
#if defined(_MSC_VER)
#else
	cds_tprim_fastsem_destroy(&ms->mSem);
	cds_tprim_eventcount_destroy(&ms->mEc);
#endif
}
void cds_tprim_monsem_wait_for_waiters(cds_tprim_monsem_t *ms, int waitForCount)
{
	int state;
	assert( waitForCount > 0 && waitForCount < CDS_TPRIM_MONSEM_WAIT_FOR_MAX );
#if defined(_MSC_VER)
#else
	state = __sync_fetch_and_add(&ms->mState, 0);
	for(;;)
	{
		int newState, ec;
		int curCount = state >> CDS_TPRIM_MONSEM_COUNT_SHIFT;
		if ( -curCount == waitForCount )
		{
			break;
		}
		
		newState = (curCount     << CDS_TPRIM_MONSEM_COUNT_SHIFT)
                 | (waitForCount << CDS_TPRIM_MONSEM_WAIT_FOR_SHIFT);
		ec = cds_tprim_eventcount_get(&ms->mEc);
		state = __sync_val_compare_and_swap(&ms->mState, state, newState);
		if (state != newState)
		{
			continue; /* retry; state was reloaded */
		}
		cds_tprim_eventcount_wait(&ms->mEc, ec);
		state = __sync_fetch_and_add(&ms->mState, 0);
	}
	for(;;)
	{
		int newState = state & CDS_TPRIM_MONSEM_COUNT_MASK;
		if (state == newState)
		{
			return;
		}
		state = __sync_val_compare_and_swap(&ms->mState, state, newState);
		if (state == newState)
		{
			return;
		}
		/* retry; state was reloaded */
	}
#endif
}
void cds_tprim_monsem_wait_no_spin(cds_tprim_monsem_t *ms)
{
#if defined(_MSC_VER)
#else
	/* int prevState = ((count-1)<<COUNT_SHIFT) | (state & WAIT_FOR_MASK); */
	int prevState = __sync_fetch_and_add(&ms->mState, (-1)<<CDS_TPRIM_MONSEM_COUNT_SHIFT);
	int prevCount = prevState >> CDS_TPRIM_MONSEM_COUNT_SHIFT; /* arithmetic shift is intentional */
	if (prevCount <= 0)
	{
		int waiters = (-prevCount) + 1;
		int waitFor = prevState & CDS_TPRIM_MONSEM_WAIT_FOR_MASK;
		assert(waiters >= 1);
		if (waiters == waitFor)
		{
			assert(waitFor >= 1);
			cds_tprim_eventcount_signal(&ms->mEc);
		}
		cds_tprim_fastsem_wait(&ms->mSem);
	}
#endif
}
int  cds_tprim_monsem_try_wait(cds_tprim_monsem_t *ms)
{
#if defined(_MSC_VER)
#else
	/* See if we can decrement the count before preparing the wait. */
	int state = __sync_fetch_and_add(&ms->mState, 0);
	for(;;)
	{
		int newState;
		if (state < (1<<CDS_TPRIM_MONSEM_COUNT_SHIFT) )
		{
			return 0;
		}
		/* newState = ((count-1)<<COUNT_SHIFT) | (state & WAIT_FOR_MASK); */
		newState = state - (1<<CDS_TPRIM_MONSEM_COUNT_SHIFT);
		assert( (newState >> CDS_TPRIM_MONSEM_COUNT_SHIFT) >= 0 );
		state = __sync_val_compare_and_swap(&ms->mState, state, newState);
		if (state == newState)
		{
			return 1;
		}
		/* state was reloaded; try again, with optional backoff. */
	}
#endif
}
int  cds_tprim_monsem_try_wait_all(cds_tprim_monsem_t *ms)
{
#if defined(_MSC_VER)
#else
	int state = __sync_fetch_and_add(&ms->mState, 0);
	for(;;)
	{
		int count, newState;
		count = state >> CDS_TPRIM_MONSEM_COUNT_SHIFT;
		if (count <= 0)
		{
			return 0;
		}
		/* zero out the count */
		newState = state & CDS_TPRIM_MONSEM_WAIT_FOR_MASK;
		state = __sync_val_compare_and_swap(&ms->mState, state, newState);
		if (state == newState)
		{
			return count;
		}
		/* state was reloaded; try again, with optional backoff. */
	}
#endif
}
void cds_tprim_monsem_wait(cds_tprim_monsem_t *ms)
{
#if defined(_MSC_VER)
#else
	int spinCount = 1; /* rrGetSpintCount() */
	while(spinCount -= 1)
	{
		if (cds_tprim_monsem_try_wait(ms))
		{
			return;
		}
	}
	cds_tprim_monsem_wait_no_spin(ms);
#endif
}
void cds_tprim_monsem_post(cds_tprim_monsem_t *ms)
{
#if defined(_MSC_VER)
#else
	int inc = 1;
	int prev = __sync_fetch_and_add(&ms->mState, inc<<CDS_TPRIM_MONSEM_COUNT_SHIFT);
	int count = (prev >> CDS_TPRIM_MONSEM_COUNT_SHIFT);
	assert(count < 0  || ( (unsigned int)count < (CDS_TPRIM_MONSEM_COUNT_MAX-2) ));
	if (count < 0)
	{
		cds_tprim_fastsem_post(&ms->mSem);
	}
#endif
}
void cds_tprim_monsem_postn(cds_tprim_monsem_t *ms, int n)
{
#if defined(_MSC_VER)
#else
	int i;
	assert(n > 0);
	for(i=0; i<n; i+=1)
	{
		cds_tprim_monsem_post(ms);
	}
#endif
}

#endif /*------------- end implementation section ---------------*/

#if defined(CDS_TPRIM_TEST)

#if defined(_MSC_VER)
#   error Self-test not supported on Windows yet.
#else
#   include <unistd.h>
#endif
#include <stdio.h>

#define CDS_TPRIM_TEST_NUM_THREADS 4

typedef struct
{
	int threadId;
	cds_tprim_fastsem_t *sem;
} test_sem_args;

static void *testSemThreadFunc(void *voidArgs)
{
	test_sem_args *args = (test_sem_args*)voidArgs;
	cds_tprim_fastsem_wait(args->sem);
	printf("Thread %d acquired the semaphore (count=%d)\n", args->threadId, cds_tprim_fastsem_getvalue(args->sem));
	sleep(5);
	cds_tprim_fastsem_post(args->sem);
	printf("Thread %d released the semaphore (count=%d)\n", args->threadId, cds_tprim_fastsem_getvalue(args->sem));
	return 0;
}

int main(int argc, char *argv[])
{
	cds_tprim_fastsem_t sem;
	pthread_t threads[CDS_TPRIM_TEST_NUM_THREADS];
	test_sem_args args[CDS_TPRIM_TEST_NUM_THREADS];
	int iThread;

	cds_tprim_fastsem_init(&sem, 2);
	for(iThread=0; iThread<CDS_TPRIM_TEST_NUM_THREADS; iThread+=1)
	{
		args[iThread].threadId = iThread;
		args[iThread].sem = &sem;
		pthread_create(&threads[iThread], NULL, testSemThreadFunc, args+iThread);
	}
	for(iThread=0; iThread<CDS_TPRIM_TEST_NUM_THREADS; iThread+=1)
	{
		pthread_join(threads[iThread], NULL);
	}

	cds_tprim_fastsem_destroy(&sem);
}
#endif /*------------------- send self-test section ------------*/

