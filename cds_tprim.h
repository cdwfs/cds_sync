/* cds_tprim.h -- portable threading/concurrency primitives in C89
 * Do this:
 *   #define CDS_TPRIM_IMPLEMENTATION
 * before including this file in *one* C/C++ file to provide the function implementations.
 *
 * For a unit test on gcc/Clang:
 *   cc -pthread -std=c89 -g -x c -DCDS_TPRIM_TEST -o [exeFile] cds_tprim.h
 * Clang users may also pass -fsanitize=thread to enable Clang's ThreadSanitizer feature.
 *
 * For a unit test on Visual C++:
 *   "%VS120COMNTOOLS%\..\..\VC\vcvarsall.bat"
 *   cl -nologo -TC -DCDS_TPRIM_TEST cds_tprim.h
 */
#if !defined(CDS_TPRIM_H)
#define CDS_TPRIM_H

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_MSC_VER)
#   define CDS_TPRIM_PLATFORM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
#   define CDS_TPRIM_PLATFORM_OSX
#elif defined(unix) || defined(__unix__) || defined(__unix)
#   include <unistd.h>
#   if defined(_POSIX_THREADS) && defined(_POSIX_SEMAPHORES)
#       define CDS_TPRIM_PLATFORM_POSIX
#   else
#       error Unsupported compliler/platform (non-POSIX unix)
#   endif
#else
#   error Unsupported compiler/platform
#endif

#if defined(CDS_TPRIM_STATIC)
#   define CDS_TPRIM_DEF static
#else
#   define CDS_TPRIM_DEF extern
#endif

#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
#   include <windows.h>
#   define CDS_TPRIM_INLINE __forceinline
#elif defined(CDS_TPRIM_PLATFORM_OSX)
#   include <pthread.h>
#   include <dispatch/dispatch.h>
#   ifdef __cplusplus
#       define CDS_TPRIM_INLINE inline
#   else
#       define CDS_TPRIM_INLINE
#   endif
#elif defined(CDS_TPRIM_PLATFORM_POSIX)
#   include <pthread.h>
#   include <semaphore.h>
#   ifdef __cplusplus
#       define CDS_TPRIM_INLINE inline
#   else
#       define CDS_TPRIM_INLINE
#   endif
#endif

    /**
     * cds_tprim_fastsem_t -- a semaphore guaranteed to stay in user code
     * unless necessary (i.e. a thread must be awakened or put to sleep).
     */
    typedef struct
    {
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
        HANDLE mHandle;
        LONG mCount;
#elif defined(CDS_TPRIM_PLATFORM_OSX)
		dispatch_semaphore_t mSem;
		int mCount;
#elif defined(CDS_TPRIM_PLATFORM_POSIX)
        sem_t mSem;
        int mCount;
#endif
    } cds_tprim_fastsem_t;

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

    /**
     * cds_tprim_eventcount_t -- an eventcount primitive, which lets you
     * indicate your interest in waiting for an event (with get()), and
     * then only actually waiting if nothing else has signaled in the
     * meantime. This ensures that the waiter only needs to wait if
     * there's legitimately no event in the queue.
     */
    typedef struct
    {
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
        CONDITION_VARIABLE mCond;
        CRITICAL_SECTION mCrit;
        LONG mCount;
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
        pthread_cond_t mCond;
        pthread_mutex_t mMtx;
        int mCount;
#endif
    } cds_tprim_eventcount_t;

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

    /**
     * cds_tprim_monsem_t -- A monitored semaphore is a semaphore that
     * allows two-sided waiting. Like a regular semaphore, a consumer can
     * decrement the semaphore and wait() for the count to be positive
     * again. A producer can increment the semaphore with post(), but can
     * also safely wait for the count to be a certain non-negative value.
     */
    typedef struct
    {
        int mState;
        cds_tprim_eventcount_t mEc;
        cds_tprim_fastsem_t mSem;
    } cds_tprim_monsem_t;

    /** @brief Initialize a monitored semaphore to the specified value. */
    CDS_TPRIM_DEF int  cds_tprim_monsem_init(cds_tprim_monsem_t *ms, int count);

    /** @brief Destroy a monitored semaphore object. */
    CDS_TPRIM_DEF void cds_tprim_monsem_destroy(cds_tprim_monsem_t *ms);

    /** @brief Waits for the semaphore value to reach a certain
     * non-negative value.
     *  @note In this implementation, only one thread can call wait_for_waiters()
     *        at a time.
     */
    CDS_TPRIM_DEF void cds_tprim_monsem_wait_for_waiters(cds_tprim_monsem_t *ms, int waitForCount);

    /** @brief Decrement the semaphore. If the result is 0 or less, put the
     *         caller to sleep.
     */
    CDS_TPRIM_DEF void cds_tprim_monsem_wait(cds_tprim_monsem_t *ms);

    /** @brief Increment the semaphore. If the counter was negative, wake a
     *         thread that was previously put to sleep by wait().
     */
    CDS_TPRIM_DEF void cds_tprim_monsem_post(cds_tprim_monsem_t *ms);

    /** @brief Increment the semaphore counter N times.
     *  @note  This is functionally equivalent to calling post() N times,
     *         but may be more efficient on some platforms.
     */
    CDS_TPRIM_DEF void cds_tprim_monsem_postn(cds_tprim_monsem_t *ms, int n);

    /**
     * cds_tprim_barrier_t -- A barrier that blocks several threads from
     * progressing until all threads have reached the barrier.
     */
    typedef struct
    {
        int insideCount, threadCount;
        cds_tprim_fastsem_t mutex, semIn, semOut;
    } cds_tprim_barrier_t;

    /** @brief Initialize a barrier object for the provided number of threads. */
    CDS_TPRIM_DEF int cds_tprim_barrier_init(cds_tprim_barrier_t *barrier, int threadCount);

    /* @brief Destroy an existing barrier object. */
    CDS_TPRIM_DEF void cds_tprim_barrier_destroy(cds_tprim_barrier_t *barrier);

    /* @brief Enters a barrier's critical section. The 1..N-1th threads to enter
     *        the barrier are put to sleep; the Nth thread wakes threads 0..N-1.
     *        Must be followed by a matching call to barrier_exit().
     * @note  Any code between barrier_enter() and barrier_exit() is a type of
     *        critical section: no thread will enter it until all threads have
     *        entered, and no thread will exit it until all threads are ready
     *        to exit.
     */
    CDS_TPRIM_DEF void cds_tprim_barrier_enter(cds_tprim_barrier_t *barrier);

    /* @brief Exits a barrier's critical section. The 1..N-1th threads to exit
     *        the barrier are put to sleep; the Nth thread wakes threads 0..N-1.
     *        Must be preceded by a matching call to barrier_enter().
     * @note  Any code between barrier_enter() and barrier_exit() is a type of
     *        critical section: no thread will enter it until all threads have
     *        entered, and no thread will exit it until all threads are ready
     *        to exit.
     */
    CDS_TPRIM_DEF void cds_tprim_barrier_exit(cds_tprim_barrier_t *barrier);

    /** @brief Helper function to call barrier_enter() and barrier_exit() in
     *         succession, if there's no code to run in the critical section.
     */
    CDS_TPRIM_DEF CDS_TPRIM_INLINE void cds_tprim_barrier_wait(cds_tprim_barrier_t *barrier)
    {
        cds_tprim_barrier_enter(barrier);
        cds_tprim_barrier_exit(barrier);
    }

#ifdef __cplusplus
}
#endif

#endif /*-------------- end header file ------------------------ */

#if defined(CDS_TPRIM_TEST)
#   if !defined(CDS_TPRIM_IMPLEMENTATION)
#       define CDS_TPRIM_IMPLEMENTATION
#   endif
#endif

#ifdef CDS_TPRIM_IMPLEMENTATION

#include <assert.h>
#include <limits.h>

#define CDS_TPRIM_MIN(a,b) ( (a)<(b) ? (a) : (b) )


/* cds_tprim_fastsem_t */
int cds_tprim_fastsem_init(cds_tprim_fastsem_t *sem, int n)
{
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    sem->mCount = n;
    sem->mHandle = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
    if (sem->mHandle == NULL)
        return GetLastError();
    return 0;
#elif defined(CDS_TPRIM_PLATFORM_OSX)
	sem->mSem = dispatch_semaphore_create(0);
	sem->mCount = n;
	return (sem->mSem != NULL) ? 0 : -1;
#elif defined(CDS_TPRIM_PLATFORM_POSIX)
    int err = sem_init(&sem->mSem, 0, 0);
    sem->mCount = n;
    return err;
#endif
}

void cds_tprim_fastsem_destroy(cds_tprim_fastsem_t *sem)
{
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    CloseHandle(sem->mHandle);
#elif defined(CDS_TPRIM_PLATFORM_OSX)
	dispatch_release(sem->mSem);
#elif defined(CDS_TPRIM_PLATFORM_POSIX)
    sem_destroy(&sem->mSem);
#endif
}

static CDS_TPRIM_INLINE int cds_tprim_fastsem_trywait(cds_tprim_fastsem_t *sem)
{
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    LONG count = sem->mCount;
    while(count > 0)
    {
        LONG newCount = count-1;
        count = InterlockedCompareExchange(&sem->mCount, newCount, count);
        if (count != newCount)
        {
            return 1;
        }
        /* otherwise, count was reloaded. backoff here is optional. */
    }
    return 0;
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
    int count = __atomic_load_n(&sem->mCount, __ATOMIC_ACQUIRE);
    while(count > 0)
    {
        if (__atomic_compare_exchange_n(&sem->mCount, &count, count-1, 1, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED))
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
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    if (InterlockedExchangeAdd(&sem->mCount, -1) < 1)
    {
        WaitForSingleObject(sem->mHandle, INFINITE);
    }
#elif defined(CDS_TPRIM_PLATFORM_OSX)
    if (__atomic_fetch_add(&sem->mCount, -1, __ATOMIC_ACQ_REL) < 1)
    {
        dispatch_semaphore_wait(sem->mSem, DISPATCH_TIME_FOREVER);
    }
#elif defined(CDS_TPRIM_PLATFORM_POSIX)
    if (__atomic_fetch_add(&sem->mCount, -1, __ATOMIC_ACQ_REL) < 1)
    {
        sem_wait(&sem->mSem);
    }
#endif
}

void cds_tprim_fastsem_wait(cds_tprim_fastsem_t *sem)
{
    int spin_count = 1;
    while(spin_count--)
    {
        if (cds_tprim_fastsem_trywait(sem) )
        {
            return;
        }
    }
    cds_tprim_fastsem_wait_no_spin(sem);
}

void cds_tprim_fastsem_post(cds_tprim_fastsem_t *sem)
{
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    if (InterlockedExchangeAdd(&sem->mCount, 1) < 0)
    {
        ReleaseSemaphore(sem->mHandle, 1, 0);
    }
#elif defined(CDS_TPRIM_PLATFORM_OSX)
	if (__atomic_fetch_add(&sem->mCount, 1, __ATOMIC_ACQ_REL) < 0)
    {
        dispatch_semaphore_signal(sem->mSem);
    }
#elif defined(CDS_TPRIM_PLATFORM_POSIX)
    if (__atomic_fetch_add(&sem->mCount, 1, __ATOMIC_ACQ_REL) < 0)
    {
        sem_post(&sem->mSem);
    }
#endif
}

void cds_tprim_fastsem_postn(cds_tprim_fastsem_t *sem, int n)
{
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    LONG oldCount = InterlockedExchangeAdd(&sem->mCount, n);
    if (oldCount < 0)
    {
        int numWaiters = -oldCount;
        int numToWake = CDS_TPRIM_MIN(numWaiters, n);
        ReleaseSemaphore(sem->mHandle, numToWake, 0);
    }
#elif defined(CDS_TPRIM_PLATFORM_OSX)
    int oldCount = __atomic_fetch_add(&sem->mCount, n, __ATOMIC_ACQ_REL);
    if (oldCount < 0)
    {
        int numWaiters = -oldCount;
        int numToWake = CDS_TPRIM_MIN(numWaiters, n);
        /* wakeN would be better than a loop here, but hey */
        int iWake;
        for(iWake=0; iWake<numToWake; iWake += 1)
        {
            dispatch_semaphore_signal(sem->mSem);
        }
    }
#elif defined(CDS_TPRIM_PLATFORM_POSIX)
    int oldCount = __atomic_fetch_add(&sem->mCount, n, __ATOMIC_ACQ_REL);
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
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    return (int)&sem->mCount;
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
    return __atomic_load_n(&sem->mCount, __ATOMIC_SEQ_CST);
#endif
}


/* cds_tprim_eventcount_t */
int cds_tprim_eventcount_init(cds_tprim_eventcount_t *ec)
{
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    InitializeConditionVariable(&ec->mCond);
    InitializeCriticalSection(&ec->mCrit);
    ec->mCount = 0;
    return 0;
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
    pthread_cond_init(&ec->mCond, 0);
    pthread_mutex_init(&ec->mMtx, 0);
    __atomic_store_n(&ec->mCount, 0, __ATOMIC_RELAXED);
    return 0;
#endif
}
void cds_tprim_eventcount_destroy(cds_tprim_eventcount_t *ec)
{
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    /* Windows CONDITION_VARIABLE object do not need to be destroyed. */
    DeleteCriticalSection(&ec->mCrit);
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
    pthread_cond_destroy(&ec->mCond);
    pthread_mutex_destroy(&ec->mMtx);
#endif
}
int cds_tprim_eventcount_get(cds_tprim_eventcount_t *ec)
{
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    return InterlockedOrAcquire(&ec->mCount, 1);
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
    return __atomic_fetch_or(&ec->mCount, 1, __ATOMIC_ACQUIRE);
#endif
}
void cds_tprim_eventcount_signal(cds_tprim_eventcount_t *ec)
{
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    LONG key = ec->mCount;
    if (key & 1)
    {
        EnterCriticalSection(&ec->mCrit);
        for(;;)
        {
            LONG newKey = (key+2) & ~1;
            key = InterlockedCompareExchange(&ec->mCount, newKey, key);
            if (key == newKey)
            {
                break;
            }
            /* spin */
        }
        LeaveCriticalSection(&ec->mCrit);
        WakeAllConditionVariable(&ec->mCond);
    }
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
    int key = __atomic_fetch_add(&ec->mCount, 0, __ATOMIC_SEQ_CST);
    if (key & 1)
    {
        pthread_mutex_lock(&ec->mMtx);
        while (!__atomic_compare_exchange_n(&ec->mCount, &key, (key+2) & ~1, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
        {
            /* spin */
        }
        pthread_mutex_unlock(&ec->mMtx);
        pthread_cond_broadcast(&ec->mCond);
    }
#endif
}
void cds_tprim_eventcount_wait(cds_tprim_eventcount_t *ec, int cmp)
{
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    EnterCriticalSection(&ec->mCrit);
    while((ec->mCount & ~1) == (cmp & ~1))
    {
        SleepConditionVariableCS(&ec->mCond, &ec->mCrit, INFINITE);
    }
    LeaveCriticalSection(&ec->mCrit);
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
    pthread_mutex_lock(&ec->mMtx);
    if ((__atomic_load_n(&ec->mCount, __ATOMIC_SEQ_CST) & ~1) == (cmp & ~1))
    {
        pthread_cond_wait(&ec->mCond, &ec->mMtx);
    }
    pthread_mutex_unlock(&ec->mMtx);
#endif
}


/* cds_tprim_monsem_t */
#define CDS_TPRIM_MONSEM_COUNT_SHIFT 8
#define CDS_TPRIM_MONSEM_COUNT_MASK  0xFFFFFF00UL
#define CDS_TPRIM_MONSEM_COUNT_MAX   ((unsigned int)(CDS_TPRIM_MONSEM_COUNT_MASK) >> (CDS_TPRIM_MONSEM_COUNT_SHIFT))
#define CDS_TPRIM_MONSEM_WAIT_FOR_SHIFT 0
#define CDS_TPRIM_MONSEM_WAIT_FOR_MASK 0xFF
#define CDS_TPRIM_MONSEM_WAIT_FOR_MAX ((CDS_TPRIM_MONSEM_WAIT_FOR_MASK) >> (CDS_TPRIM_MONSEM_WAIT_FOR_SHIFT))

int cds_tprim_monsem_init(cds_tprim_monsem_t *ms, int count)
{
    assert(count >= 0);
    cds_tprim_fastsem_init(&ms->mSem, 0);
    cds_tprim_eventcount_init(&ms->mEc);
    ms->mState = count << CDS_TPRIM_MONSEM_COUNT_SHIFT;
    return 0;
}
void cds_tprim_monsem_destroy(cds_tprim_monsem_t *ms)
{
    cds_tprim_fastsem_destroy(&ms->mSem);
    cds_tprim_eventcount_destroy(&ms->mEc);
}

void cds_tprim_monsem_wait_for_waiters(cds_tprim_monsem_t *ms, int waitForCount)
{
    int state;
    assert( waitForCount > 0 && waitForCount < CDS_TPRIM_MONSEM_WAIT_FOR_MAX );
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    state = ms->mState;
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
        state = InterlockedCompareExchange(&ms->mState, newState, state);
        if (state != newState)
        {
            continue; /* retry; state was reloaded */
        }
        cds_tprim_eventcount_wait(&ms->mEc, ec);
        state = ms->mState;
    }
    for(;;)
    {
        int newState = state & CDS_TPRIM_MONSEM_COUNT_MASK;
        if (state == newState)
        {
            return; /* nothing to do */
        }
        state = InterlockedCompareExchange(&ms->mState, newState, state);
        if (state == newState)
        {
            return; /* updated successfully */
        }
        /* retry; state was reloaded */
    }
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
    state = __atomic_load_n(&ms->mState, __ATOMIC_ACQUIRE);
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
        if (!__atomic_compare_exchange_n(&ms->mState, &state, newState, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
        {
            continue; /* retry; state was reloaded */
        }
        cds_tprim_eventcount_wait(&ms->mEc, ec);
        state = __atomic_load_n(&ms->mState, __ATOMIC_ACQUIRE);
    }
    for(;;)
    {
        int newState = state & CDS_TPRIM_MONSEM_COUNT_MASK;
        if (state == newState)
        {
            return; /* nothing to do */
        }
        if (__atomic_compare_exchange_n(&ms->mState, &state, newState, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
        {
            return; /* updated successfully */
        }
        /* retry; state was reloaded */
    }
#endif
}

static CDS_TPRIM_INLINE void cds_tprim_monsem_wait_no_spin(cds_tprim_monsem_t *ms)
{
    /* int prevState = ((count-1)<<COUNT_SHIFT) | (state & WAIT_FOR_MASK); */
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    int prevState = InterlockedExchangeAdd(&ms->mState, (-1)<<CDS_TPRIM_MONSEM_COUNT_SHIFT);
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
    int prevState = __atomic_fetch_add(&ms->mState, (-1)<<CDS_TPRIM_MONSEM_COUNT_SHIFT, __ATOMIC_ACQ_REL);
#endif

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
}

static CDS_TPRIM_INLINE int cds_tprim_monsem_try_wait(cds_tprim_monsem_t *ms)
{
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    /* See if we can decrement the count before preparing the wait. */
    int state = ms->mState;
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
        state = InterlockedCompareExchange(&ms->mState, newState, state);
        if (state == newState)
        {
            return 1;
        }
        /* state was reloaded; try again, with optional backoff. */
    }
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
    /* See if we can decrement the count before preparing the wait. */
    int state = __atomic_load_n(&ms->mState, __ATOMIC_ACQUIRE);
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
        if (__atomic_compare_exchange_n(&ms->mState, &state, newState, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
        {
            return 1;
        }
        /* state was reloaded; try again, with optional backoff. */
    }
#endif
}

static CDS_TPRIM_INLINE int cds_tprim_monsem_try_wait_all(cds_tprim_monsem_t *ms)
{
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    int state = ms->mState;
    for(;;)
    {
        int newState;
        int count = state >> CDS_TPRIM_MONSEM_COUNT_SHIFT;
        if (count <= 0)
        {
            return 0;
        }
        /* zero out the count */
        newState = state & CDS_TPRIM_MONSEM_WAIT_FOR_MASK;
        state = InterlockedCompareExchange(&ms->mState, newState, state);
        if (state == newState)
        {
            return count;
        }
        /* state was reloaded; try again, with optional backoff. */
    }
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
    int state = __atomic_load_n(&ms->mState, __ATOMIC_ACQUIRE);
    for(;;)
    {
        int newState;
        int count = state >> CDS_TPRIM_MONSEM_COUNT_SHIFT;
        if (count <= 0)
        {
            return 0;
        }
        /* zero out the count */
        newState = state & CDS_TPRIM_MONSEM_WAIT_FOR_MASK;
        if (__atomic_compare_exchange_n(&ms->mState, &state, newState, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
        {
            return count;
        }
        /* state was reloaded; try again, with optional backoff. */
    }
#endif
}

void cds_tprim_monsem_wait(cds_tprim_monsem_t *ms)
{
    int spinCount = 1;
    while(spinCount -= 1)
    {
        if (cds_tprim_monsem_try_wait(ms))
        {
            return;
        }
    }
    cds_tprim_monsem_wait_no_spin(ms);
}

void cds_tprim_monsem_post(cds_tprim_monsem_t *ms)
{
    const int inc = 1;
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    int prev = InterlockedExchangeAdd(&ms->mState, inc<<CDS_TPRIM_MONSEM_COUNT_SHIFT);
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
    int prev = __atomic_fetch_add(&ms->mState, inc<<CDS_TPRIM_MONSEM_COUNT_SHIFT, __ATOMIC_ACQ_REL);
#endif
    int count = (prev >> CDS_TPRIM_MONSEM_COUNT_SHIFT);
    assert(count < 0  || ( (unsigned int)count < (CDS_TPRIM_MONSEM_COUNT_MAX-2) ));
    if (count < 0)
    {
        cds_tprim_fastsem_post(&ms->mSem);
    }
}

void cds_tprim_monsem_postn(cds_tprim_monsem_t *ms, int n)
{
    int i;
    assert(n > 0);
    for(i=0; i<n; i+=1)
    {
        cds_tprim_monsem_post(ms);
    }
}


/* cds_tprim_barrier_t */
int cds_tprim_barrier_init(cds_tprim_barrier_t *barrier, int threadCount)
{
    assert(threadCount > 0);
    barrier->insideCount = 0;
    barrier->threadCount = threadCount;
    cds_tprim_fastsem_init(&barrier->mutex, 1);
    cds_tprim_fastsem_init(&barrier->semIn, 0);
    cds_tprim_fastsem_init(&barrier->semOut, 0);
    return 0;
}

void cds_tprim_barrier_destroy(cds_tprim_barrier_t *barrier)
{
    cds_tprim_fastsem_destroy(&barrier->mutex);
    cds_tprim_fastsem_destroy(&barrier->semIn);
    cds_tprim_fastsem_destroy(&barrier->semOut);
}

void cds_tprim_barrier_enter(cds_tprim_barrier_t *barrier)
{
    cds_tprim_fastsem_wait(&barrier->mutex);
    barrier->insideCount += 1;
    if (barrier->insideCount == barrier->threadCount)
    {
        cds_tprim_fastsem_postn(&barrier->semIn, barrier->threadCount);
    }
    cds_tprim_fastsem_post(&barrier->mutex);
    cds_tprim_fastsem_wait(&barrier->semIn);
}

void cds_tprim_barrier_exit(cds_tprim_barrier_t *barrier)
{
    cds_tprim_fastsem_wait(&barrier->mutex);
    barrier->insideCount -= 1;
    if (barrier->insideCount == 0)
    {
        cds_tprim_fastsem_postn(&barrier->semOut, barrier->threadCount);
    }
    cds_tprim_fastsem_post(&barrier->mutex);
    cds_tprim_fastsem_wait(&barrier->semOut);
}


#endif /*------------- end implementation section ---------------*/

#if defined(CDS_TPRIM_TEST)

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
typedef HANDLE cds_tprim_thread_t;
typedef DWORD cds_tprim_threadproc_return_t;
static CDS_TPRIM_INLINE void cds_tprim_thread_create(cds_tprim_thread_t *pThread, LPTHREAD_START_ROUTINE startProc, void *args) { *pThread = CreateThread(NULL,0,startProc,args,0,NULL); }
static CDS_TPRIM_INLINE void cds_tprim_thread_join(cds_tprim_thread_t thread) { WaitForSingleObject(thread, INFINITE); CloseHandle(thread); }
static CDS_TPRIM_INLINE int cds_tprim_thread_id(void) { return (int)GetCurrentThreadId(); }
#   define CDS_TPRIM_ATOMIC_LOAD(pDest) (*(pDest))
#   define CDS_TPRIM_ATOMIC_FETCH_ADD(pDest,n) InterlockedExchangeAdd( (pDest), (n) )
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
#   include <unistd.h>
typedef pthread_t cds_tprim_thread_t;
typedef void* cds_tprim_threadproc_return_t;
static CDS_TPRIM_INLINE void cds_tprim_thread_create(cds_tprim_thread_t *pThread, void *(*startProc)(void*), void *args) { pthread_create(pThread,NULL,startProc,args); }
static CDS_TPRIM_INLINE void cds_tprim_thread_join(cds_tprim_thread_t thread) { pthread_join(thread, NULL); }
static CDS_TPRIM_INLINE int cds_tprim_thread_id(void) { return (int)pthread_self(); }
#   define CDS_TPRIM_ATOMIC_LOAD(pDest)      __atomic_load_n( (pDest), __ATOMIC_SEQ_CST )
#   define CDS_TPRIM_ATOMIC_FETCH_ADD(pDest,n) __atomic_fetch_add( (pDest), (n), __ATOMIC_SEQ_CST )
#endif

#define CDS_TPRIM_TEST_NUM_THREADS 16
static int g_errorCount = 0;

#define CDS_TEST_DANCER_NUM_ROUNDS 1000
typedef struct
{
    struct
    {
        int leaderId;
        int followerId;
    } rounds[CDS_TEST_DANCER_NUM_ROUNDS];
    cds_tprim_fastsem_t queueL, queueF, mutexL, mutexF, rendezvous;
    int roundIndex;
} cds_test_dancer_args_t;
static cds_tprim_threadproc_return_t testDancerLeader(void *voidArgs)
{
    cds_test_dancer_args_t *args = (cds_test_dancer_args_t*)voidArgs;
    int threadId = cds_tprim_thread_id();
    int lastRoundIndex = 0;
    int zero = 0;
    for(;;)
    {
        zero = 0;
        cds_tprim_fastsem_wait(&args->mutexL);
        cds_tprim_fastsem_post(&args->queueL);
        cds_tprim_fastsem_wait(&args->queueF);
        /* critical section */
        if (args->roundIndex < CDS_TEST_DANCER_NUM_ROUNDS)
        {
            zero = CDS_TPRIM_ATOMIC_FETCH_ADD(&args->rounds[args->roundIndex].leaderId, threadId);
        }
        cds_tprim_fastsem_wait(&args->rendezvous);
        lastRoundIndex = CDS_TPRIM_ATOMIC_FETCH_ADD(&args->roundIndex, 1);
        /* end critical section */
        cds_tprim_fastsem_post(&args->mutexL);
        if (0 != zero)
        {
            printf("ERROR: double-write to rounds[%d].leaderId (expected 0, found %d)\n",
                lastRoundIndex, zero);
            ++g_errorCount;
        }
        if (lastRoundIndex+1 >= CDS_TEST_DANCER_NUM_ROUNDS)
        {
            break;
        }
    }
    return (cds_tprim_threadproc_return_t)NULL;
}
static cds_tprim_threadproc_return_t testDancerFollower(void *voidArgs)
{
    cds_test_dancer_args_t *args = (cds_test_dancer_args_t*)voidArgs;
    int threadId = cds_tprim_thread_id();
    int lastRoundIndex = 0;
    int zero = 0;
    for(;;)
    {
        zero = 0;
        cds_tprim_fastsem_wait(&args->mutexF);
        cds_tprim_fastsem_post(&args->queueF);
        cds_tprim_fastsem_wait(&args->queueL);
        /* critical section */
        lastRoundIndex = CDS_TPRIM_ATOMIC_LOAD(&args->roundIndex);
        if (args->roundIndex < CDS_TEST_DANCER_NUM_ROUNDS)
        {
            zero = CDS_TPRIM_ATOMIC_FETCH_ADD(&args->rounds[args->roundIndex].followerId, threadId);
        }
        cds_tprim_fastsem_post(&args->rendezvous);
        /* end critical section */
        cds_tprim_fastsem_post(&args->mutexF);
        if (0 != zero)
        {
            printf("ERROR: double-write to rounds[%d].followererId (expected 0, found %d)\n",
                lastRoundIndex, zero);
            ++g_errorCount;
        }
        if (lastRoundIndex+1 >= CDS_TEST_DANCER_NUM_ROUNDS)
        {
            break;
        }
    }
    return (cds_tprim_threadproc_return_t)NULL;
}

int main(int argc, char *argv[])
{
    /*
     * Use semaphores to implement a pair of queues: one for leaders,
     * one for followers. The queue protects a critical section, which
     * should be entered by exactly one of each type at a time.
     */
    {
        cds_test_dancer_args_t dancerArgs;
        cds_tprim_thread_t *leaderThreads = NULL, *followerThreads = NULL;
        const int kNumLeaders = 8, kNumFollowers = 8;
        int iLeader=0, iFollower=0, iRound=0;

        dancerArgs.roundIndex = 0;
        memset(dancerArgs.rounds, 0, sizeof(dancerArgs.rounds));
        cds_tprim_fastsem_init(&dancerArgs.queueL, 0);
        cds_tprim_fastsem_init(&dancerArgs.queueF, 0);
        cds_tprim_fastsem_init(&dancerArgs.mutexL, 1);
        cds_tprim_fastsem_init(&dancerArgs.mutexF, 1);
        cds_tprim_fastsem_init(&dancerArgs.rendezvous, 0);

        leaderThreads = (cds_tprim_thread_t*)malloc(kNumLeaders*sizeof(cds_tprim_thread_t));
        for(iLeader=0; iLeader<kNumLeaders; ++iLeader)
        {
            cds_tprim_thread_create(&leaderThreads[iLeader], testDancerLeader, &dancerArgs);
        }
        followerThreads = (cds_tprim_thread_t*)malloc(kNumFollowers*sizeof(cds_tprim_thread_t));
        for(iFollower=0; iFollower<kNumFollowers; ++iFollower)
        {
            cds_tprim_thread_create(&followerThreads[iFollower], testDancerFollower, &dancerArgs);
        }

        for(iLeader=0; iLeader<kNumLeaders; ++iLeader)
        {
            cds_tprim_thread_join(leaderThreads[iLeader]);
        }
        for(iFollower=0; iFollower<kNumFollowers; ++iFollower)
        {
            cds_tprim_thread_join(followerThreads[iFollower]);
        }

        /* verify that each round's leader/follower IDs are non-zero. */
        for(iRound=0; iRound<CDS_TEST_DANCER_NUM_ROUNDS; ++iRound)
        {
            if (0 == dancerArgs.rounds[iRound].leaderId ||
                0 == dancerArgs.rounds[iRound].followerId)
            {
                printf("ERROR: round[%d] = [%d,%d]\n", iRound,
                    dancerArgs.rounds[iRound].leaderId,
                    dancerArgs.rounds[iRound].followerId);
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
