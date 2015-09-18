/* cds_tprim.h -- portable threading/concurrency primitives in C89
 * Do this:
 *   #define CDS_TPRIM_IMPLEMENTATION
 * before including this file in *one* C/C++ file to provide the function
 * implementations.
 *
 * For a unit test on gcc/Clang:
 *   cc -pthread -std=c89 -g -x c -DCDS_TPRIM_TEST -o test_tprim.exe cds_tprim.h
 * Clang users may also pass -fsanitize=thread to enable Clang's
 * ThreadSanitizer feature.
 *
 * For a unit test on Visual C++:
 *   "%VS120COMNTOOLS%\..\..\VC\vcvarsall.bat"
 *   cl -nologo -TC -DCDS_TPRIM_TEST cds_tprim.h
 * Debug-mode:
 *   cl -Od -Z7 -FC -MTd -nologo -TC -DCDS_TPRIM_TEST cds_tprim.h
 */

/* TODO:
 * - move to feature tests (CDS_TPRIM_HAS_XXX), not platform tests.
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
#       error Unsupported compiler/platform (non-POSIX unix)
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
#   define CDS_TPRIM_INLINE __forceinline
#elif defined(CDS_TPRIM_PLATFORM_OSX)
#   ifdef __cplusplus
#       define CDS_TPRIM_INLINE inline
#   else
#       define CDS_TPRIM_INLINE
#   endif
#elif defined(CDS_TPRIM_PLATFORM_POSIX)
#   ifdef __cplusplus
#       define CDS_TPRIM_INLINE inline
#   else
#       define CDS_TPRIM_INLINE
#   endif
#endif

#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
#   include <windows.h>
#elif defined(CDS_TPRIM_PLATFORM_OSX)
#   include <pthread.h>
#   include <dispatch/dispatch.h>
#elif defined(CDS_TPRIM_PLATFORM_POSIX)
#   include <pthread.h>
#   include <semaphore.h>
#endif

#if defined(_MSC_VER) && (_MSC_VER < 1700)
    /* no stdint.h in VS2010 and earlier.
     * LONG and DWORD are guaranteed to be 32 bits forever, though.
     */
    typedef LONG  cds_tprim_s32;
    typedef DWORD cds_tprim_u32;
#else
#   include <stdint.h>
    typedef int32_t  cds_tprim_s32;
    typedef uint32_t cds_tprim_u32;
#endif


    /**
     * cds_tprim_fastsem_t -- a semaphore guaranteed to stay in user code
     * unless necessary (i.e. a thread must be awakened or put to sleep).
     */
    typedef struct
    {
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
        HANDLE handle;
#elif defined(CDS_TPRIM_PLATFORM_OSX)
        dispatch_semaphore_t sem;
#elif defined(CDS_TPRIM_PLATFORM_POSIX)
        sem_t sem;
#endif
        cds_tprim_s32 count;
    } cds_tprim_fastsem_t;

    /** @brief Initialize a semaphore to the specified value. */
    CDS_TPRIM_DEF int cds_tprim_fastsem_init(cds_tprim_fastsem_t *sem, cds_tprim_s32 n);

    /** @brief Destroy an existing semaphore object. */
    CDS_TPRIM_DEF void cds_tprim_fastsem_destroy(cds_tprim_fastsem_t *sem);

    /** @brief Decrement the semaphore's internal counter. If the
     *         counter is <=0, this call will block until it is
     *         positive again before decrementing.
     * @return 0 if the semaphore was initialized successfully; non-zero
     *         if an error occurred.
     */
    CDS_TPRIM_DEF void cds_tprim_fastsem_wait(cds_tprim_fastsem_t *sem);

    /** @brief Increment the semaphore's internal counter. If the counter is
     *         <=0, this call will wake a thread that had previously called
     *         sem_wait().
     */
    CDS_TPRIM_DEF void cds_tprim_fastsem_post(cds_tprim_fastsem_t *sem);

    /** @brief Functionally equivalent to calling sem_post() n
     *         times. This variant may be more efficient if the
     *         underlying OS semaphore allows multiple threads to be
     *         awakened with a single kernel call.
     */
    CDS_TPRIM_DEF void cds_tprim_fastsem_postn(cds_tprim_fastsem_t *sem, cds_tprim_s32 n);

    /** @brief Retrieves the current value of the semaphore's internal
     *         counter, for debugging purposes. If the counter is
     *         positive, it represents the number of available
     *         resources that have been posted. If it's negative, it
     *         is the (negated) number of threads waiting for a
     *         resource to be posted.
     *  @return The current value of the semaphore's internal counter.
     */
    CDS_TPRIM_DEF cds_tprim_s32 cds_tprim_fastsem_getvalue(cds_tprim_fastsem_t *sem);

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
        CONDITION_VARIABLE cond;
        CRITICAL_SECTION crit;
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
        pthread_cond_t cond;
        pthread_mutex_t mtx;
#endif
        cds_tprim_s32 count;
    } cds_tprim_eventcount_t;

    /** @brief Initialize an eventcount object. */
    CDS_TPRIM_DEF int cds_tprim_eventcount_init(cds_tprim_eventcount_t *ec);

    /** @brief Destroy an eventcount object. */
    CDS_TPRIM_DEF void cds_tprim_eventcount_destroy(cds_tprim_eventcount_t *ec);

    /** @brief Return the current event count, in preparation for a
     *         call to wait().  Often called prepare_wait().
     *  @note This is *not* a simple query function; the internal
     *        object state is modified.
     */
    CDS_TPRIM_DEF cds_tprim_s32 cds_tprim_eventcount_get(cds_tprim_eventcount_t *ec);

    /** @brief Wakes all threads waiting on an eventcount object, and
     *         increment the internal counter.  If no waiters are
     *         registered, this function is a no-op.
     */
    CDS_TPRIM_DEF void cds_tprim_eventcount_signal(cds_tprim_eventcount_t *ec);

    /** @brief Compare the provided count to the eventcount object's
     *         internal counter; if they match, put the calling thread
     *         to sleep until the eventcount is signalled.  If the
     *         counts do not match, the caller returns more-or-less
     *         immediately.
     */
    CDS_TPRIM_DEF void cds_tprim_eventcount_wait(cds_tprim_eventcount_t *ec, cds_tprim_s32 cmp);

    /**
     * cds_tprim_monsem_t -- A monitored semaphore is a semaphore that
     * allows two-sided waiting. Like a regular semaphore, a consumer can
     * decrement the semaphore and wait() for the count to be positive
     * again. A producer can increment the semaphore with post(), but can
     * also safely wait for the count to be a certain non-negative value.
     */
    typedef struct
    {
        cds_tprim_s32 state;
        cds_tprim_eventcount_t ec;
        cds_tprim_fastsem_t sem;
    } cds_tprim_monsem_t;

    /** @brief Initialize a monitored semaphore to the specified value. */
    CDS_TPRIM_DEF int  cds_tprim_monsem_init(cds_tprim_monsem_t *ms, cds_tprim_s32 count);

    /** @brief Destroy a monitored semaphore object. */
    CDS_TPRIM_DEF void cds_tprim_monsem_destroy(cds_tprim_monsem_t *ms);

    /** @brief Waits for the semaphore value to reach a certain
     *         non-negative value.
     *  @note In this implementation, only one thread can call
     *        wait_for_waiters() at a time.
     */
    CDS_TPRIM_DEF void cds_tprim_monsem_wait_for_waiters(cds_tprim_monsem_t *ms, cds_tprim_s32 waitForCount);

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
    CDS_TPRIM_DEF void cds_tprim_monsem_postn(cds_tprim_monsem_t *ms, cds_tprim_s32 n);

    /**
     * cds_tprim_barrier_t -- A barrier that blocks several threads from
     * progressing until all threads have reached the barrier.
     */
    typedef struct
    {
        cds_tprim_s32 insideCount, threadCount;
        cds_tprim_fastsem_t mutex, semIn, semOut;
    } cds_tprim_barrier_t;

    /** @brief Initialize a barrier object for the provided number of
     *         threads.
     */
    CDS_TPRIM_DEF int cds_tprim_barrier_init(cds_tprim_barrier_t *barrier, cds_tprim_s32 threadCount);

    /** @brief Destroy an existing barrier object. */
    CDS_TPRIM_DEF void cds_tprim_barrier_destroy(cds_tprim_barrier_t *barrier);

    /** @brief Enters a barrier's critical section. The 1..N-1th
     *         threads to enter the barrier are put to sleep; the Nth
     *         thread wakes threads 0..N-1.  Must be followed by a
     *         matching call to barrier_exit().
     * @note Any code between barrier_enter() and barrier_exit() is a
     *       type of critical section: no thread will enter it until
     *       all threads have entered, and no thread will exit it
     *       until all threads are ready to exit.
     */
    CDS_TPRIM_DEF void cds_tprim_barrier_enter(cds_tprim_barrier_t *barrier);

    /* @brief Exits a barrier's critical section. The 1..N-1th threads
     *        to exit the barrier are put to sleep; the Nth thread
     *        wakes threads 0..N-1.  Must be preceded by a matching
     *        call to barrier_enter().
     * @note Any code between barrier_enter() and barrier_exit() is a
     *       type of critical section: no thread will enter it until
     *       all threads have entered, and no thread will exit it
     *       until all threads are ready to exit.
     */
    CDS_TPRIM_DEF void cds_tprim_barrier_exit(cds_tprim_barrier_t *barrier);

    /** @brief Helper function to call barrier_enter() and
     *         barrier_exit() in succession, if there's no code to run
     *         in the critical section.
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
#include <stdint.h>

#define CDS_TPRIM_MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define CDS_TPRIM_UNUSED(x) ((void)x)

/* Silly Windows, not providing gcc-style atomic intrinsics. This is
 * not an exhaustive implementation; it's only the subset used by
 * cds_tprim. */
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
#   define CDS_TPRIM_ATOMIC_SEQ_CST  0
#   define CDS_TPRIM_ATOMIC_ACQUIRE  1
#   define CDS_TPRIM_ATOMIC_RELAXED  2
#   define CDS_TPRIM_ATOMIC_ACQ_REL  3
static CDS_TPRIM_INLINE cds_tprim_s32 cds_tprim_atomic_load_s32(cds_tprim_s32 *ptr, int memorder) { CDS_TPRIM_UNUSED(memorder); return *ptr; }
static CDS_TPRIM_INLINE void    cds_tprim_atomic_store_s32(cds_tprim_s32 *ptr, cds_tprim_s32 val, int memorder) { CDS_TPRIM_UNUSED(memorder); *ptr = val; }
static CDS_TPRIM_INLINE cds_tprim_s32 cds_tprim_atomic_fetch_add_s32(cds_tprim_s32 *ptr, cds_tprim_s32 val, int memorder)
{
    switch(memorder)
    {
    case CDS_TPRIM_ATOMIC_ACQUIRE:
        return InterlockedExchangeAddAcquire(ptr, val);
    case CDS_TPRIM_ATOMIC_RELAXED:
        return InterlockedExchangeAddNoFence(ptr, val);
    case CDS_TPRIM_ATOMIC_SEQ_CST:
    case CDS_TPRIM_ATOMIC_ACQ_REL:
        return InterlockedExchangeAdd(ptr, val);
    default:
        assert(0); /* unsupported memory order */
        return InterlockedExchangeAdd(ptr, val);
    }
}
static CDS_TPRIM_INLINE cds_tprim_s32 cds_tprim_atomic_fetch_or_s32(cds_tprim_s32 *ptr, cds_tprim_s32 val, int memorder)
{
    switch(memorder)
    {
    case CDS_TPRIM_ATOMIC_ACQUIRE:
        return InterlockedOrAcquire(ptr, val);
    case CDS_TPRIM_ATOMIC_RELAXED:
        return InterlockedOrNoFence(ptr, val);
    case CDS_TPRIM_ATOMIC_SEQ_CST:
    case CDS_TPRIM_ATOMIC_ACQ_REL:
        return InterlockedOr(ptr, val);
    default:
        assert(0); /* unsupported memory order */
        return InterlockedOr(ptr, val);
    }
}
static CDS_TPRIM_INLINE int cds_tprim_atomic_compare_exchange_s32(cds_tprim_s32 *ptr, cds_tprim_s32 *expected, cds_tprim_s32 desired,
    int weak, int success_memorder, int failure_memorder)
{
    /* TODO: try to match code generated by gcc/clang intrinsic */
    cds_tprim_s32 original;
    int success;
    CDS_TPRIM_UNUSED(weak);
    CDS_TPRIM_UNUSED(failure_memorder);
    switch(success_memorder)
    {
    case CDS_TPRIM_ATOMIC_ACQUIRE:
        original = InterlockedCompareExchangeAcquire(ptr, desired, *expected);
        break;
    case CDS_TPRIM_ATOMIC_RELAXED:
        original = InterlockedCompareExchangeNoFence(ptr, desired, *expected);
        break;
    case CDS_TPRIM_ATOMIC_SEQ_CST:
    case CDS_TPRIM_ATOMIC_ACQ_REL:
        original = InterlockedCompareExchange(ptr, desired, *expected);
        break;
    default:
        assert(0); /* unsupported memory order */
        original = InterlockedCompareExchange(ptr, desired, *expected);
        break;
    }
    success = (original == cds_tprim_atomic_load_s32(expected, CDS_TPRIM_ATOMIC_SEQ_CST)) ? 1 : 0;
    cds_tprim_atomic_store_s32(expected, original, CDS_TPRIM_ATOMIC_SEQ_CST);
    return success;
}
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
#   define CDS_TPRIM_ATOMIC_SEQ_CST  __ATOMIC_SEQ_CST
#   define CDS_TPRIM_ATOMIC_ACQUIRE  __ATOMIC_ACQUIRE
#   define CDS_TPRIM_ATOMIC_RELAXED  __ATOMIC_RELAXED
#   define CDS_TPRIM_ATOMIC_ACQ_REL  __ATOMIC_ACQ_REL
static CDS_TPRIM_INLINE cds_tprim_s32 cds_tprim_atomic_load_s32(cds_tprim_s32 *ptr, int memorder) { return __atomic_load_n(ptr, memorder); }
static CDS_TPRIM_INLINE void    cds_tprim_atomic_store_s32(cds_tprim_s32 *ptr, cds_tprim_s32 val, int memorder) { return __atomic_store_n(ptr, val, memorder); }
static CDS_TPRIM_INLINE cds_tprim_s32 cds_tprim_atomic_fetch_add_s32(cds_tprim_s32 *ptr, cds_tprim_s32 val, int memorder) { return __atomic_fetch_add(ptr, val, memorder); }
static CDS_TPRIM_INLINE cds_tprim_s32 cds_tprim_atomic_fetch_or_s32(cds_tprim_s32 *ptr, cds_tprim_s32 val, int memorder) { return __atomic_fetch_or(ptr, val, memorder); }
static CDS_TPRIM_INLINE int cds_tprim_atomic_compare_exchange_s32(cds_tprim_s32 *ptr, cds_tprim_s32 *expected, cds_tprim_s32 desired,
    int weak, int success_memorder, int failure_memorder)
{
    return __atomic_compare_exchange_n(ptr, expected, desired, weak, success_memorder, failure_memorder);
}
#endif

/* cds_tprim_fastsem_t */
int cds_tprim_fastsem_init(cds_tprim_fastsem_t *sem, cds_tprim_s32 n)
{
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    sem->count = n;
    sem->handle = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
    if (sem->handle == NULL)
        return GetLastError();
    return 0;
#elif defined(CDS_TPRIM_PLATFORM_OSX)
    sem->sem = dispatch_semaphore_create(0);
    sem->count = n;
    return (sem->sem != NULL) ? 0 : -1;
#elif defined(CDS_TPRIM_PLATFORM_POSIX)
    int err = sem_init(&sem->sem, 0, 0);
    sem->count = n;
    return err;
#endif
}

void cds_tprim_fastsem_destroy(cds_tprim_fastsem_t *sem)
{
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    CloseHandle(sem->handle);
#elif defined(CDS_TPRIM_PLATFORM_OSX)
    dispatch_release(sem->sem);
#elif defined(CDS_TPRIM_PLATFORM_POSIX)
    sem_destroy(&sem->sem);
#endif
}

static CDS_TPRIM_INLINE int cds_tprim_fastsem_trywait(cds_tprim_fastsem_t *sem)
{
    cds_tprim_s32 count = cds_tprim_atomic_load_s32(&sem->count, CDS_TPRIM_ATOMIC_ACQUIRE);
    while(count > 0)
    {
        if (cds_tprim_atomic_compare_exchange_s32(&sem->count, &count, count-1,
                1, CDS_TPRIM_ATOMIC_ACQ_REL, CDS_TPRIM_ATOMIC_RELAXED))
        {
            return 1;
        }
        /* otherwise, count was reloaded. backoff here is optional. */
    }
    return 0;
}

static CDS_TPRIM_INLINE void cds_tprim_fastsem_wait_no_spin(cds_tprim_fastsem_t *sem)
{
    if (cds_tprim_atomic_fetch_add_s32(&sem->count, -1, CDS_TPRIM_ATOMIC_ACQ_REL) < 1)
    {
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
        WaitForSingleObject(sem->handle, INFINITE);
#elif defined(CDS_TPRIM_PLATFORM_OSX)
        dispatch_semaphore_wait(sem->sem, DISPATCH_TIME_FOREVER);
#elif defined(CDS_TPRIM_PLATFORM_POSIX)
        sem_wait(&sem->sem);
#endif
    }
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
    if (cds_tprim_atomic_fetch_add_s32(&sem->count, 1, CDS_TPRIM_ATOMIC_ACQ_REL) < 0)
    {
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
        ReleaseSemaphore(sem->handle, 1, 0);
#elif defined(CDS_TPRIM_PLATFORM_OSX)
        dispatch_semaphore_signal(sem->sem);
#elif defined(CDS_TPRIM_PLATFORM_POSIX)
        sem_post(&sem->sem);
#endif
    }
}

void cds_tprim_fastsem_postn(cds_tprim_fastsem_t *sem, cds_tprim_s32 n)
{
    cds_tprim_s32 oldCount = cds_tprim_atomic_fetch_add_s32(&sem->count, n, CDS_TPRIM_ATOMIC_ACQ_REL);
    if (oldCount < 0)
    {
        cds_tprim_s32 numWaiters = -oldCount;
        cds_tprim_s32 numToWake = CDS_TPRIM_MIN(numWaiters, n);
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
        ReleaseSemaphore(sem->handle, numToWake, 0);
#elif defined(CDS_TPRIM_PLATFORM_OSX)
        /* wakeN would be better than a loop here, but hey */
        cds_tprim_s32 iWake;
        for(iWake=0; iWake<numToWake; iWake += 1)
        {
            dispatch_semaphore_signal(sem->sem);
        }
#elif defined(CDS_TPRIM_PLATFORM_POSIX)
        /* wakeN would be better than a loop here, but hey */
        cds_tprim_s32 iWake;
        for(iWake=0; iWake<numToWake; iWake += 1)
        {
            sem_post(&sem->sem);
        }
#endif
    }
}

cds_tprim_s32 cds_tprim_fastsem_getvalue(cds_tprim_fastsem_t *sem)
{
    return cds_tprim_atomic_load_s32(&sem->count, CDS_TPRIM_ATOMIC_SEQ_CST);
}


/* cds_tprim_eventcount_t */
int cds_tprim_eventcount_init(cds_tprim_eventcount_t *ec)
{
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    InitializeConditionVariable(&ec->cond);
    InitializeCriticalSection(&ec->crit);
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
    pthread_cond_init(&ec->cond, 0);
    pthread_mutex_init(&ec->mtx, 0);
#endif
    cds_tprim_atomic_store_s32(&ec->count, 0, CDS_TPRIM_ATOMIC_RELAXED);
    return 0;
}
void cds_tprim_eventcount_destroy(cds_tprim_eventcount_t *ec)
{
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    /* Windows CONDITION_VARIABLE object do not need to be destroyed. */
    DeleteCriticalSection(&ec->crit);
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
    pthread_cond_destroy(&ec->cond);
    pthread_mutex_destroy(&ec->mtx);
#endif
}
cds_tprim_s32 cds_tprim_eventcount_get(cds_tprim_eventcount_t *ec)
{
    return cds_tprim_atomic_fetch_or_s32(&ec->count, 1, CDS_TPRIM_ATOMIC_ACQUIRE);
}
void cds_tprim_eventcount_signal(cds_tprim_eventcount_t *ec)
{
    cds_tprim_s32 key = cds_tprim_atomic_fetch_add_s32(&ec->count, 0, CDS_TPRIM_ATOMIC_SEQ_CST);
    if (key & 1)
    {
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
        EnterCriticalSection(&ec->crit);
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
        pthread_mutex_lock(&ec->mtx);
#endif
        while (!cds_tprim_atomic_compare_exchange_s32(&ec->count, &key, (key+2) & ~1,
                1, CDS_TPRIM_ATOMIC_SEQ_CST, CDS_TPRIM_ATOMIC_SEQ_CST))
        {
            /* spin */
        }
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
        LeaveCriticalSection(&ec->crit);
        WakeAllConditionVariable(&ec->cond);
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
        pthread_mutex_unlock(&ec->mtx);
        pthread_cond_broadcast(&ec->cond);
#endif
    }
}
void cds_tprim_eventcount_wait(cds_tprim_eventcount_t *ec, cds_tprim_s32 cmp)
{
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    EnterCriticalSection(&ec->crit);
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
    pthread_mutex_lock(&ec->mtx);
#endif
    if ((cds_tprim_atomic_load_s32(&ec->count, CDS_TPRIM_ATOMIC_SEQ_CST) & ~1) == (cmp & ~1))
    {
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
        SleepConditionVariableCS(&ec->cond, &ec->crit, INFINITE);
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
        pthread_cond_wait(&ec->cond, &ec->mtx);
#endif
    }
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
    LeaveCriticalSection(&ec->crit);
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
    pthread_mutex_unlock(&ec->mtx);
#endif
}


/* cds_tprim_monsem_t */
#define CDS_TPRIM_MONSEM_COUNT_SHIFT 8
#define CDS_TPRIM_MONSEM_COUNT_MASK  0xFFFFFF00UL
#define CDS_TPRIM_MONSEM_COUNT_MAX   ((cds_tprim_u32)(CDS_TPRIM_MONSEM_COUNT_MASK) >> (CDS_TPRIM_MONSEM_COUNT_SHIFT))
#define CDS_TPRIM_MONSEM_WAIT_FOR_SHIFT 0
#define CDS_TPRIM_MONSEM_WAIT_FOR_MASK 0xFF
#define CDS_TPRIM_MONSEM_WAIT_FOR_MAX ((CDS_TPRIM_MONSEM_WAIT_FOR_MASK) >> (CDS_TPRIM_MONSEM_WAIT_FOR_SHIFT))

int cds_tprim_monsem_init(cds_tprim_monsem_t *ms, cds_tprim_s32 count)
{
    assert(count >= 0);
    cds_tprim_fastsem_init(&ms->sem, 0);
    cds_tprim_eventcount_init(&ms->ec);
    ms->state = count << CDS_TPRIM_MONSEM_COUNT_SHIFT;
    return 0;
}
void cds_tprim_monsem_destroy(cds_tprim_monsem_t *ms)
{
    cds_tprim_fastsem_destroy(&ms->sem);
    cds_tprim_eventcount_destroy(&ms->ec);
}

void cds_tprim_monsem_wait_for_waiters(cds_tprim_monsem_t *ms, cds_tprim_s32 waitForCount)
{
    cds_tprim_s32 state;
    assert( waitForCount > 0 && waitForCount < CDS_TPRIM_MONSEM_WAIT_FOR_MAX );
    state = cds_tprim_atomic_load_s32(&ms->state, CDS_TPRIM_ATOMIC_ACQUIRE);
    for(;;)
    {
        cds_tprim_s32 newState, ec;
        cds_tprim_s32 curCount = state >> CDS_TPRIM_MONSEM_COUNT_SHIFT;
        if ( -curCount == waitForCount )
        {
            break;
        }
        newState = (curCount     << CDS_TPRIM_MONSEM_COUNT_SHIFT)
            | (waitForCount << CDS_TPRIM_MONSEM_WAIT_FOR_SHIFT);
        ec = cds_tprim_eventcount_get(&ms->ec);
        if (!cds_tprim_atomic_compare_exchange_s32(&ms->state, &state, newState,
                0, CDS_TPRIM_ATOMIC_ACQ_REL, CDS_TPRIM_ATOMIC_ACQUIRE))
        {
            continue; /* retry; state was reloaded */
        }
        cds_tprim_eventcount_wait(&ms->ec, ec);
        state = cds_tprim_atomic_load_s32(&ms->state, CDS_TPRIM_ATOMIC_ACQUIRE);
    }
    for(;;)
    {
        cds_tprim_s32 newState = state & CDS_TPRIM_MONSEM_COUNT_MASK;
        if (state == newState)
        {
            return; /* nothing to do */
        }
        if (cds_tprim_atomic_compare_exchange_s32(&ms->state, &state, newState,
                0, CDS_TPRIM_ATOMIC_ACQ_REL, CDS_TPRIM_ATOMIC_ACQUIRE))
        {
            return; /* updated successfully */
        }
        /* retry; state was reloaded */
    }
}

static CDS_TPRIM_INLINE void cds_tprim_monsem_wait_no_spin(cds_tprim_monsem_t *ms)
{
    /* cds_tprim_s32 prevState = ((count-1)<<COUNT_SHIFT) | (state & WAIT_FOR_MASK); */
    cds_tprim_s32 prevState = cds_tprim_atomic_fetch_add_s32(&ms->state, (-1)<<CDS_TPRIM_MONSEM_COUNT_SHIFT,
        CDS_TPRIM_ATOMIC_ACQ_REL);

    cds_tprim_s32 prevCount = prevState >> CDS_TPRIM_MONSEM_COUNT_SHIFT; /* arithmetic shift is intentional */
    if (prevCount <= 0)
    {
        cds_tprim_s32 waiters = (-prevCount) + 1;
        cds_tprim_s32 waitFor = prevState & CDS_TPRIM_MONSEM_WAIT_FOR_MASK;
        assert(waiters >= 1);
        if (waiters == waitFor)
        {
            assert(waitFor >= 1);
            cds_tprim_eventcount_signal(&ms->ec);
        }
        cds_tprim_fastsem_wait(&ms->sem);
    }
}

static CDS_TPRIM_INLINE int cds_tprim_monsem_try_wait(cds_tprim_monsem_t *ms)
{
    /* See if we can decrement the count before preparing the wait. */
    cds_tprim_s32 state = cds_tprim_atomic_load_s32(&ms->state, CDS_TPRIM_ATOMIC_ACQUIRE);
    for(;;)
    {
        cds_tprim_s32 newState;
        if (state < (1<<CDS_TPRIM_MONSEM_COUNT_SHIFT) )
        {
            return 0;
        }
        /* newState = ((count-1)<<COUNT_SHIFT) | (state & WAIT_FOR_MASK); */
        newState = state - (1<<CDS_TPRIM_MONSEM_COUNT_SHIFT);
        assert( (newState >> CDS_TPRIM_MONSEM_COUNT_SHIFT) >= 0 );
        if (cds_tprim_atomic_compare_exchange_s32(&ms->state, &state, newState,
                0, CDS_TPRIM_ATOMIC_ACQ_REL, CDS_TPRIM_ATOMIC_ACQUIRE))
        {
            return 1;
        }
        /* state was reloaded; try again, with optional backoff. */
    }
}

static CDS_TPRIM_INLINE int cds_tprim_monsem_try_wait_all(cds_tprim_monsem_t *ms)
{
    cds_tprim_s32 state = cds_tprim_atomic_load_s32(&ms->state, CDS_TPRIM_ATOMIC_ACQUIRE);
    for(;;)
    {
        cds_tprim_s32 newState;
        cds_tprim_s32 count = state >> CDS_TPRIM_MONSEM_COUNT_SHIFT;
        if (count <= 0)
        {
            return 0;
        }
        /* zero out the count */
        newState = state & CDS_TPRIM_MONSEM_WAIT_FOR_MASK;
        if (cds_tprim_atomic_compare_exchange_s32(&ms->state, &state, newState,
                0, CDS_TPRIM_ATOMIC_ACQ_REL, CDS_TPRIM_ATOMIC_ACQUIRE))
        {
            return count;
        }
        /* state was reloaded; try again, with optional backoff. */
    }
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
    const cds_tprim_s32 inc = 1;
    cds_tprim_s32 prev = cds_tprim_atomic_fetch_add_s32(&ms->state, inc<<CDS_TPRIM_MONSEM_COUNT_SHIFT,
        CDS_TPRIM_ATOMIC_ACQ_REL);
    cds_tprim_s32 count = (prev >> CDS_TPRIM_MONSEM_COUNT_SHIFT);
    assert(count < 0  || ( (unsigned int)count < (CDS_TPRIM_MONSEM_COUNT_MAX-2) ));
    if (count < 0)
    {
        cds_tprim_fastsem_post(&ms->sem);
    }
}

void cds_tprim_monsem_postn(cds_tprim_monsem_t *ms, cds_tprim_s32 n)
{
    cds_tprim_s32 i;
    assert(n > 0);
    for(i=0; i<n; i+=1)
    {
        cds_tprim_monsem_post(ms);
    }
}


/* cds_tprim_barrier_t */
int cds_tprim_barrier_init(cds_tprim_barrier_t *barrier, cds_tprim_s32 threadCount)
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

/* Minimal thread management wrappers */
#if defined(CDS_TPRIM_PLATFORM_WINDOWS)
typedef HANDLE cds_tprim_thread_t;
typedef DWORD cds_tprim_threadproc_return_t;
#   define CDS_TPRIM_THREADPROC WINAPI
static CDS_TPRIM_INLINE void cds_tprim_thread_create(cds_tprim_thread_t *pThread, LPTHREAD_START_ROUTINE startProc, void *args) { *pThread = CreateThread(NULL,0,startProc,args,0,NULL); }
static CDS_TPRIM_INLINE void cds_tprim_thread_join(cds_tprim_thread_t thread) { WaitForSingleObject(thread, INFINITE); CloseHandle(thread); }
static CDS_TPRIM_INLINE int cds_tprim_thread_id(void) { return (int)GetCurrentThreadId(); }
static CDS_TPRIM_INLINE void cds_tprim_sleep_ms(int ms) { Sleep(ms); }
#elif defined(CDS_TPRIM_PLATFORM_OSX) || defined(CDS_TPRIM_PLATFORM_POSIX)
typedef pthread_t cds_tprim_thread_t;
typedef void* cds_tprim_threadproc_return_t;
#   define CDS_TPRIM_THREADPROC
static CDS_TPRIM_INLINE void cds_tprim_thread_create(cds_tprim_thread_t *pThread, void *(*startProc)(void*), void *args) { pthread_create(pThread,NULL,startProc,args); }
static CDS_TPRIM_INLINE void cds_tprim_thread_join(cds_tprim_thread_t thread) { pthread_join(thread, NULL); }
static CDS_TPRIM_INLINE int cds_tprim_thread_id(void) { return (int)pthread_self(); }
static CDS_TPRIM_INLINE void cds_tprim_sleep_ms(int ms) { uleep(1000*ms); }
#endif

static cds_tprim_s32 g_errorCount = 0;

#define CDS_TEST_DANCER_NUM_ROUNDS 1000
typedef struct
{
    struct
    {
        cds_tprim_s32 leaderId;
        cds_tprim_s32 followerId;
    } rounds[CDS_TEST_DANCER_NUM_ROUNDS];
    cds_tprim_fastsem_t queueL, queueF, mutexL, mutexF, rendezvous;
    cds_tprim_s32 roundIndex;
} cds_test_dancer_args_t;
static cds_tprim_threadproc_return_t CDS_TPRIM_THREADPROC testDancerLeader(void *voidArgs)
{
    cds_test_dancer_args_t *args = (cds_test_dancer_args_t*)voidArgs;
    int threadId = cds_tprim_thread_id();
    cds_tprim_s32 lastRoundIndex = 0;
    cds_tprim_s32 zero = 0;
    for(;;)
    {
        zero = 0;
        cds_tprim_fastsem_wait(&args->mutexL);
        cds_tprim_fastsem_post(&args->queueL);
        cds_tprim_fastsem_wait(&args->queueF);
        /* critical section */
        if (args->roundIndex < CDS_TEST_DANCER_NUM_ROUNDS)
        {
            zero = cds_tprim_atomic_fetch_add_s32(&args->rounds[args->roundIndex].leaderId, threadId,
                CDS_TPRIM_ATOMIC_SEQ_CST);
        }
        cds_tprim_fastsem_wait(&args->rendezvous);
        lastRoundIndex = cds_tprim_atomic_fetch_add_s32(&args->roundIndex, 1, CDS_TPRIM_ATOMIC_SEQ_CST);
        /* end critical section */
        cds_tprim_fastsem_post(&args->mutexL);
        if (0 != zero)
        {
            printf("ERROR: double-write to rounds[%d].leaderId (expected 0, found %d)\n",
                lastRoundIndex, zero);
            cds_tprim_atomic_fetch_add_s32(&g_errorCount, 1, CDS_TPRIM_ATOMIC_SEQ_CST);
        }
        if (lastRoundIndex+1 >= CDS_TEST_DANCER_NUM_ROUNDS)
        {
            break;
        }
    }
    return (cds_tprim_threadproc_return_t)NULL;
}

static cds_tprim_threadproc_return_t CDS_TPRIM_THREADPROC testDancerFollower(void *voidArgs)
{
    cds_test_dancer_args_t *args = (cds_test_dancer_args_t*)voidArgs;
    int threadId = cds_tprim_thread_id();
    cds_tprim_s32 lastRoundIndex = 0;
    cds_tprim_s32 zero = 0;
    for(;;)
    {
        zero = 0;
        cds_tprim_fastsem_wait(&args->mutexF);
        cds_tprim_fastsem_post(&args->queueF);
        cds_tprim_fastsem_wait(&args->queueL);
        /* critical section */
        lastRoundIndex = cds_tprim_atomic_load_s32(&args->roundIndex, CDS_TPRIM_ATOMIC_SEQ_CST);
        if (args->roundIndex < CDS_TEST_DANCER_NUM_ROUNDS)
        {
            zero = cds_tprim_atomic_fetch_add_s32(&args->rounds[args->roundIndex].followerId, threadId,
                CDS_TPRIM_ATOMIC_SEQ_CST);
        }
        cds_tprim_fastsem_post(&args->rendezvous);
        /* end critical section */
        cds_tprim_fastsem_post(&args->mutexF);
        if (0 != zero)
        {
            printf("ERROR: double-write to rounds[%d].followererId (expected 0, found %d)\n",
                lastRoundIndex, zero);
            cds_tprim_atomic_fetch_add_s32(&g_errorCount, 1, CDS_TPRIM_ATOMIC_SEQ_CST);
        }
        if (lastRoundIndex+1 >= CDS_TEST_DANCER_NUM_ROUNDS)
        {
            break;
        }
    }
    return (cds_tprim_threadproc_return_t)NULL;
}

static void testDancers(void)
{
    /*
     * Use semaphores to implement a pair of queues: one for leaders,
     * one for followers. The queue protects a critical section, which
     * should be entered by exactly one of each type at a time.
     */
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

#define CDS_TEST_QUEUE_LENGTH 100
typedef struct cds_test_queue_args_t
{
    cds_tprim_fastsem_t mtx; /* protects queue; too lazy to write a lockless */
    cds_tprim_eventcount_t ec;
    cds_tprim_s32 input[CDS_TEST_QUEUE_LENGTH];
    cds_tprim_s32 output[CDS_TEST_QUEUE_LENGTH];
    cds_tprim_s32 readIndex, writeIndex;
} cds_test_queue_args_t;
static cds_tprim_threadproc_return_t CDS_TPRIM_THREADPROC testQueuePopper(void *voidArgs)
{
    cds_test_queue_args_t *args = (cds_test_queue_args_t*)voidArgs;
    cds_tprim_s32 n = 0;
    while(n >= 0 && args->readIndex < CDS_TEST_QUEUE_LENGTH)
    {
        cds_tprim_s32 count = 0;
        int gotOne = 0;
        cds_tprim_s32 ri, wi;
        cds_tprim_fastsem_wait(&args->mtx);
        if (args->readIndex < args->writeIndex)
        {
            n = args->input[args->readIndex++];
            gotOne = 1;
        }
        cds_tprim_fastsem_post(&args->mtx);
        if (gotOne)
        {
            if (args->output[abs(n)] != 0)
                printf("0x%08X: output[%3d] = %3d (expected 0)\n", cds_tprim_thread_id(), abs(n), args->output[abs(n)]);
            args->output[abs(n)] += n;
            continue;
        }
        /* queue was empty; get() the EC and try again */
        count = cds_tprim_eventcount_get(&args->ec);
        cds_tprim_fastsem_wait(&args->mtx);
        if (args->readIndex < args->writeIndex)
        {
            n = args->input[args->readIndex++];
            gotOne = 1;
        }
        cds_tprim_fastsem_post(&args->mtx);
        if (gotOne)
        {
            /* TODO: cancel EC get()? */
            if (args->output[abs(n)] != 0)
                printf("0x%08X: output[%3d] = %3d (expected 0)\n", cds_tprim_thread_id(), abs(n), args->output[abs(n)]);
            args->output[abs(n)] += n;
            continue;
        }
        /* queue still empty; wait until it has something available. */
        cds_tprim_eventcount_wait(&args->ec, count);
    }
    return 0;
}
static void testEventcount(void)
{
    cds_test_queue_args_t args;
    cds_tprim_s32 iEntry, iPopper;
    cds_tprim_thread_t *popperThreads = NULL;
    const int kNumPoppers = 8;

    cds_tprim_fastsem_init(&args.mtx, 1);
    cds_tprim_eventcount_init(&args.ec);
    for(iEntry=0; iEntry<CDS_TEST_QUEUE_LENGTH; ++iEntry)
    {
        args.input[iEntry]  = (iEntry+kNumPoppers < CDS_TEST_QUEUE_LENGTH) ? iEntry : -iEntry;
        args.output[iEntry] = 0;
    }
    args.readIndex = 0;
    args.writeIndex = 0;

    popperThreads = (cds_tprim_thread_t*)malloc(kNumPoppers*sizeof(cds_tprim_thread_t));
    for(iPopper=0; iPopper<kNumPoppers; ++iPopper)
    {
        cds_tprim_thread_create(&popperThreads[iPopper], testQueuePopper, &args);
    }
    
    iEntry=0;
    for(;;)
    {
        cds_tprim_s32 numPushes = rand() % 4 + 1;
        int sleepMs = rand() % 2;
        if (args.writeIndex + numPushes > CDS_TEST_QUEUE_LENGTH)
            numPushes = CDS_TEST_QUEUE_LENGTH - args.writeIndex;
        cds_tprim_atomic_fetch_add_s32(&args.writeIndex, numPushes, CDS_TPRIM_ATOMIC_SEQ_CST);
        cds_tprim_eventcount_signal(&args.ec);
        if (args.writeIndex == CDS_TEST_QUEUE_LENGTH)
        {
            break;
        }
        cds_tprim_sleep_ms(sleepMs);
    }
    
    for(iPopper=0; iPopper<kNumPoppers; ++iPopper)
    {
        cds_tprim_thread_join(popperThreads[iPopper]);
    }
    for(iEntry=0; iEntry<CDS_TEST_QUEUE_LENGTH; ++iEntry)
    {
        if (abs(args.output[iEntry]) != iEntry)
        {
            printf("ERROR: double-write to output[%d]; expected %d, found %d\n", iEntry, iEntry, args.output[iEntry]);
            ++g_errorCount;
        }
    }
}

int main(int argc, char *argv[])
{
    int seed = time(NULL);
    printf("random seed: 0x%08X\n");
    srand(seed);

    for(;;)
    {
        testDancers();
        printf("error count after testDancers():    %d\n", g_errorCount);
        
        testEventcount();
        printf("error count after testEventcount(): %d\n", g_errorCount);
    }
}
#endif /*------------------- send self-test section ------------*/
