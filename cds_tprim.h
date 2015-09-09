/* cds_tprim.h -- portable threading/concurrency primitives in C89
 * Do this:
 *   #define CDS_TPRIM_IMPLEMENTATION
 * before including this file in *one* C/C++ file to provide the function implementations.
 *
 * For a unit test:
 *   cc -lpthread -std=c89 -g -x c -DCDS_TPRIM_TEST -o [exeFile] cds_tprim.h
 * Clang users may also pass -fsanitize=thread to enable Clang's ThreadSanitizer feature.
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

static CDS_TPRIM_INLINE int cds_tprim_fastsem_trywait(cds_tprim_fastsem_t *sem)
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

static CDS_TPRIM_INLINE void cds_tprim_fastsem_wait_no_spin(cds_tprim_fastsem_t *sem)
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

int cds_tprim_monsem_init(cds_tprim_monsem_t *ms, int count)
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

static CDS_TPRIM_INLINE void cds_tprim_monsem_wait_no_spin(cds_tprim_monsem_t *ms)
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

static CDS_TPRIM_INLINE int cds_tprim_monsem_try_wait(cds_tprim_monsem_t *ms)
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

static CDS_TPRIM_INLINE int cds_tprim_monsem_try_wait_all(cds_tprim_monsem_t *ms)
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define CDS_TPRIM_TEST_NUM_THREADS 16
static int g_errorCount = 0;

#define CDS_TEST_DANCER_NUM_ROUNDS 1000
typedef struct
{
	struct
    {
        intptr_t leaderId;
        intptr_t followerId;
    } rounds[CDS_TEST_DANCER_NUM_ROUNDS];
	cds_tprim_fastsem_t queueL, queueF, mutexL, mutexF, rendezvous;
	int roundIndex;
} cds_test_dancer_args_t;
static void *testDancerLeader(void *voidArgs)
{
    cds_test_dancer_args_t *args = (cds_test_dancer_args_t*)voidArgs;
	intptr_t threadId = (intptr_t)pthread_self();
    int lastRoundIndex = 0;
    intptr_t zero = 0;
    for(;;)
    {
        zero = 0;
        cds_tprim_fastsem_wait(&args->mutexL);
        cds_tprim_fastsem_post(&args->queueL);
        cds_tprim_fastsem_wait(&args->queueF);
        /* critical section */
        if (args->roundIndex < CDS_TEST_DANCER_NUM_ROUNDS)
        {
            zero = __sync_fetch_and_add(&args->rounds[args->roundIndex].leaderId, threadId);
        }
        cds_tprim_fastsem_wait(&args->rendezvous);
        lastRoundIndex = __sync_fetch_and_add(&args->roundIndex, 1);
        /* end critical section */
        cds_tprim_fastsem_post(&args->mutexL);
        if (0 != zero)
        {
            printf("ERROR: double-write to rounds[%d].leaderId (expected 0, found %p)\n",
                lastRoundIndex, (void*)zero);
            ++g_errorCount;
        }
        if (lastRoundIndex+1 >= CDS_TEST_DANCER_NUM_ROUNDS)
        {
            break;
        }
    }
    return NULL;
}
static void *testDancerFollower(void *voidArgs)
{
    cds_test_dancer_args_t *args = (cds_test_dancer_args_t*)voidArgs;
	intptr_t threadId = (intptr_t)pthread_self();
    int lastRoundIndex = 0;
    intptr_t zero = 0;
    for(;;)
    {
        zero = 0;
        cds_tprim_fastsem_wait(&args->mutexF);
        cds_tprim_fastsem_post(&args->queueF);
        cds_tprim_fastsem_wait(&args->queueL);
        /* critical section */
        lastRoundIndex = __sync_fetch_and_add(&args->roundIndex, 0);
        if (args->roundIndex < CDS_TEST_DANCER_NUM_ROUNDS)
        {
            zero = __sync_fetch_and_add(&args->rounds[args->roundIndex].followerId, threadId);
        }
        cds_tprim_fastsem_post(&args->rendezvous);
        /* end critical section */
        cds_tprim_fastsem_post(&args->mutexF);
        if (0 != zero)
        {
            printf("ERROR: double-write to rounds[%d].followererId (expected 0, found %p)\n",
                lastRoundIndex, (void*)zero);
            ++g_errorCount;
        }
        if (lastRoundIndex+1 >= CDS_TEST_DANCER_NUM_ROUNDS)
        {
            break;
        }
    }
	return NULL;
}

int main(int argc, char *argv[])
{
	/*
	 * Use semaphores to implement a pair of queues: one for leaders,
	 * one for followers. The queue protects a critical section, which
	 * should be entered by exactly one of each type at a time.
	 */
	{
		cds_test_dancer_args_t dancerArgs = {};
		pthread_t *leaderThreads = NULL, *followerThreads = NULL;
		const int kNumLeaders = 8, kNumFollowers = 8;
		int iLeader=0, iFollower=0, iRound=0;
		
        memset(dancerArgs.rounds, 0, sizeof(dancerArgs.rounds));
		cds_tprim_fastsem_init(&dancerArgs.queueL, 0);
		cds_tprim_fastsem_init(&dancerArgs.queueF, 0);
		cds_tprim_fastsem_init(&dancerArgs.mutexL, 1);
		cds_tprim_fastsem_init(&dancerArgs.mutexF, 1);
		cds_tprim_fastsem_init(&dancerArgs.rendezvous, 0);

		leaderThreads = (pthread_t*)malloc(kNumLeaders*sizeof(pthread_t));
		for(iLeader=0; iLeader<kNumLeaders; ++iLeader)
		{
			pthread_create(&leaderThreads[iLeader], NULL,
				testDancerLeader, &dancerArgs);
		}
		followerThreads = (pthread_t*)malloc(kNumFollowers*sizeof(pthread_t));
		for(iFollower=0; iFollower<kNumFollowers; ++iFollower)
		{
			pthread_create(&followerThreads[iFollower], NULL,
				testDancerFollower, &dancerArgs);
		}

		for(iLeader=0; iLeader<kNumLeaders; ++iLeader)
		{
			pthread_join(leaderThreads[iLeader], NULL);
		}
		for(iFollower=0; iFollower<kNumFollowers; ++iFollower)
		{
			pthread_join(followerThreads[iFollower], NULL);
		}

		/* verify that each round's leader/follower IDs are non-zero. */
		for(iRound=0; iRound<CDS_TEST_DANCER_NUM_ROUNDS; ++iRound)
		{
			if (0 == dancerArgs.rounds[iRound].leaderId ||
				0 == dancerArgs.rounds[iRound].followerId)
			{
				printf("ERROR: round[%d] = [%p,%p]\n", iRound,
					(void*)dancerArgs.rounds[iRound].leaderId,
					(void*)dancerArgs.rounds[iRound].followerId);
				++g_errorCount;
			}
		}

		cds_tprim_fastsem_destroy(&dancerArgs.queueL);
		cds_tprim_fastsem_destroy(&dancerArgs.queueF);
		cds_tprim_fastsem_destroy(&dancerArgs.mutexL);
		cds_tprim_fastsem_destroy(&dancerArgs.mutexF);
		cds_tprim_fastsem_destroy(&dancerArgs.rendezvous);
		free(leaderThreads);
		free(followerThreads);
	}
    printf("error count after dancers: %d\n", g_errorCount);
}
#endif /*------------------- send self-test section ------------*/

