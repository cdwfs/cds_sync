/* cds_sync.h -- portable threading/concurrency primitives in C89
 *               No warranty implied; use at your own risk.
 *
 * Do this:
 *   #define CDS_SYNC_IMPLEMENTATION
 * before including this file in *one* C/C++ file to provide the function
 * implementations.
 *
 * For a unit test on gcc/Clang:
 *   cc -Wall -pthread -std=c89 -D_POSIX_C_SOURCE=199309L -g -x c -DCDS_SYNC_TEST -o test_cds_sync.exe cds_sync.h
 * Clang users may also pass -fsanitize=thread to enable Clang's
 * ThreadSanitizer feature.
 *
 * For a unit test on Visual C++:
 *   "%VS120COMNTOOLS%\..\..\VC\vcvarsall.bat"
 *   cl -W4 -nologo -TC -DCDS_SYNC_TEST /Fetest_cds_sync.exe cds_sync.h
 * Debug-mode:
 *   cl -W4 -Od -Z7 -FC -MTd -nologo -TC -DCDS_SYNC_TEST /Fetest_cds_sync.exe cds_sync.h
 *
 * LICENSE:
 * This software is in the public domain. Where that dedication is not
 * recognized, you are granted a perpetual, irrevocable license to
 * copy, distribute, and modify this file as you see fit.
 */

#if !defined(CDS_SYNC_H)
#define CDS_SYNC_H

#ifdef __cplusplus
extern "C"
{
#endif

#if   defined(_MSC_VER)
#   define CDS_SYNC_PLATFORM_WINDOWS
#   define CDS_SYNC_HAS_WINDOWS_SEMAPHORES
#   define CDS_SYNC_HAS_WINDOWS_CONDVARS
#   define CDS_SYNC_HAS_WINDOWS_ATOMICS
#   define CDS_SYNC_HAS_WINDOWS_THREADS
#   define CDS_SYNC_HAS_WINDOWS_YIELD
#elif defined(__APPLE__) && defined(__MACH__)
#   define CDS_SYNC_PLATFORM_OSX
#   define CDS_SYNC_HAS_GCD_SEMAPHORES
#   define CDS_SYNC_HAS_POSIX_CONDVARS
#   define CDS_SYNC_HAS_GCC_ATOMICS
#   define CDS_SYNC_HAS_POSIX_THREADS
#   define CDS_SYNC_HAS_PTHREAD_YIELD_NP
#elif defined(unix) || defined(__unix__) || defined(__unix)
#   include <unistd.h>
#   if defined(_POSIX_THREADS) && defined(_POSIX_SEMAPHORES)
#       define CDS_SYNC_PLATFORM_POSIX
#       define CDS_SYNC_HAS_POSIX_SEMAPHORES
#       define CDS_SYNC_HAS_POSIX_CONDVARS
#       define CDS_SYNC_HAS_GCC_ATOMICS
#       define CDS_SYNC_HAS_POSIX_THREADS
#       define CDS_SYNC_HAS_SCHED_YIELD
#   else
#       error Unsupported platform (non-POSIX unix)
#   endif
#else
#   error Unsupported platform
#endif

#if defined(_MSC_VER)
#   define CDS_SYNC_COMPILER_MSVC
#elif defined(__clang__)
#   define CDS_SYNC_COMPILER_CLANG
#elif defined(__GNUC__)
#   define CDS_SYNC_COMPILER_GCC
#else
#   error Unsupported compiler
#endif

#if defined(CDS_SYNC_STATIC)
#   define CDS_SYNC_DEF static
#else
#   define CDS_SYNC_DEF extern
#endif

#if   defined(CDS_SYNC_PLATFORM_WINDOWS)
#   define CDS_SYNC_INLINE __forceinline
#elif defined(CDS_SYNC_PLATFORM_OSX)
#   ifdef __cplusplus
#       define CDS_SYNC_INLINE inline
#   else
#       define CDS_SYNC_INLINE
#   endif
#elif defined(CDS_SYNC_PLATFORM_POSIX)
#   ifdef __cplusplus
#       define CDS_SYNC_INLINE inline
#   else
#       define CDS_SYNC_INLINE
#   endif
#endif

#if   defined(CDS_SYNC_HAS_WINDOWS_SEMAPHORES)
#   include <windows.h>
#elif defined(CDS_SYNC_HAS_GCD_SEMAPHORES)
#   include <dispatch/dispatch.h>
#elif defined(CDS_SYNC_HAS_POSIX_SEMAPHORES)
#   include <semaphore.h>
#else
#   error Unsupported compiler/platform
#endif

#if defined(CDS_SYNC_HAS_WINDOWS_CONDVARS)
    /* windows.h already included */
#elif defined(CDS_SYNC_HAS_POSIX_CONDVARS)
#   include <pthread.h>
#else
#   error Unsupported compiler/platform
#endif

#if defined(_MSC_VER) && (_MSC_VER < 1700)
    /* no stdint.h in VS2010 and earlier.
     * LONG and DWORD are guaranteed to be 32 bits forever, though.
     */
    typedef INT8  cds_sync_s8;
    typedef BYTE  cds_sync_u8;
    typedef LONG  cds_sync_s32;
    typedef DWORD cds_sync_u32;
#else
#   include <stdint.h>
    typedef  int8_t  cds_sync_s8;
    typedef uint8_t  cds_sync_u8;
    typedef  int32_t cds_sync_s32;
    typedef uint32_t cds_sync_u32;
#endif

#if defined(CDS_SYNC_COMPILER_MSVC)
#   define CDS_SYNC_ALIGN(n) __declspec(align(n))
#elif defined(CDS_SYNC_COMPILER_CLANG) || defined(CDS_SYNC_COMPILER_GCC)
#   define CDS_SYNC_ALIGN(n) __attribute__((__aligned__(n)))
#else
#   error Unsupporter compiler
#endif
#define CDS_SYNC_L1_CACHE_LINE_SIZE 64
#define CDS_SYNC_CACHE_ALIGNED CDS_SYNC_ALIGN(CDS_SYNC_L1_CACHE_LINE_SIZE)
    /**
     * cds_sync_atomic_s32 -- a 32-bit integer in a cache line all by itself, so
     * that atomic operations don't cause false sharing.
     */
    typedef struct
    {
        CDS_SYNC_CACHE_ALIGNED cds_sync_s32 n;
        cds_sync_u8 padding[CDS_SYNC_L1_CACHE_LINE_SIZE-sizeof(cds_sync_s32)];
    } cds_sync_atomic_s32;

    /**
     * cds_sync_fusem_t -- a "fast userspace" semaphore, guaranteed
     * to stay in user code unless necessary (i.e. a thread must be
     * awakened or put to sleep).
     */
    typedef struct
    {
        cds_sync_atomic_s32 count;
#if   defined(CDS_SYNC_HAS_WINDOWS_SEMAPHORES)
        HANDLE handle;
#elif defined(CDS_SYNC_HAS_GCD_SEMAPHORES)
        dispatch_semaphore_t sem;
#elif defined(CDS_SYNC_HAS_POSIX_SEMAPHORES)
        sem_t sem;
#else
#   error Unsupported compiler/platform
#endif
    } cds_sync_fusem_t;

    /** @brief Initialize a semaphore to the specified value. */
    CDS_SYNC_DEF
    int cds_sync_fusem_init(cds_sync_fusem_t *sem, cds_sync_s32 n);

    /** @brief Destroy an existing semaphore object. */
    CDS_SYNC_DEF
    void cds_sync_fusem_destroy(cds_sync_fusem_t *sem);

    /** @brief Decrement the semaphore's internal counter. If the
     *         counter is <=0, this call will block until it is
     *         positive again before decrementing.
     * @return 0 if the semaphore was initialized successfully; non-zero
     *         if an error occurred.
     */
    CDS_SYNC_DEF
    void cds_sync_fusem_wait(cds_sync_fusem_t *sem);

    /** @brief Increment the semaphore's internal counter. If the counter is
     *         <=0, this call will wake a thread that had previously called
     *         sem_wait().
     */
    CDS_SYNC_DEF
    void cds_sync_fusem_post(cds_sync_fusem_t *sem);

    /** @brief Functionally equivalent to calling sem_post() n
     *         times. This variant may be more efficient if the
     *         underlying OS semaphore allows multiple threads to be
     *         awakened with a single kernel call.
     */
    CDS_SYNC_DEF
    void cds_sync_fusem_postn(cds_sync_fusem_t *sem, cds_sync_s32 n);

    /** @brief Retrieves the current value of the semaphore's internal
     *         counter, for debugging purposes. If the counter is
     *         positive, it represents the number of available
     *         resources that have been posted. If it's negative, it
     *         is the (negated) number of threads waiting for a
     *         resource to be posted.
     *  @return The current value of the semaphore's internal counter. NOTE:
     *          Note that, by design, the value returned may be incorrect
     *          before the calling code even sees it.
     */
    CDS_SYNC_DEF
    cds_sync_s32 cds_sync_fusem_getvalue(cds_sync_fusem_t *sem);

    /**
     * cds_sync_futex_t -- a mutex guaranteed to stay in user-mode unless
     * necessary (i.e. a thread must be awaked or put to sleep).
     */
    typedef struct
    {
        cds_sync_fusem_t sem;
    } cds_sync_futex_t;

    CDS_SYNC_DEF CDS_SYNC_INLINE
    int cds_sync_futex_init(cds_sync_futex_t *ftx)
    {
        return cds_sync_fusem_init(&ftx->sem, 1);
    }

    CDS_SYNC_DEF CDS_SYNC_INLINE
    void cds_sync_futex_destroy(cds_sync_futex_t *ftx)
    {
        cds_sync_fusem_destroy(&ftx->sem);
    }

    CDS_SYNC_DEF CDS_SYNC_INLINE
    void cds_sync_futex_lock(cds_sync_futex_t *ftx)
    {
        cds_sync_fusem_wait(&ftx->sem);
    }

    CDS_SYNC_DEF CDS_SYNC_INLINE
    void cds_sync_futex_unlock(cds_sync_futex_t *ftx)
    {
        cds_sync_fusem_post(&ftx->sem);
    }

    /**
     * cds_sync_monitor_t -- currently just a portable wrapper around
     * platform-specific condition variable and mutex implementations,
     * with no extra fancy functionality.
     */
    typedef struct
    {
#if defined(CDS_SYNC_HAS_WINDOWS_CONDVARS)
        CONDITION_VARIABLE cv;
        CRITICAL_SECTION crit;
#elif defined(CDS_SYNC_HAS_POSIX_CONDVARS)
        pthread_cond_t cv;
        pthread_mutex_t mtx;
#else
#   error Unsupported compiler/platform
#endif
    } cds_sync_monitor_t;

    CDS_SYNC_DEF CDS_SYNC_INLINE
    int cds_sync_monitor_init(cds_sync_monitor_t *mon)
    {
#if   defined(CDS_SYNC_HAS_WINDOWS_CONDVARS)
        InitializeConditionVariable(&mon->cv);
        InitializeCriticalSection(&mon->crit);
#elif defined(CDS_SYNC_HAS_POSIX_CONDVARS)
        pthread_cond_init(&mon->cv, 0);
        pthread_mutex_init(&mon->mtx, 0);
#else
#   error Unsupported compiler/platform
#endif
        return 0;
    }

    CDS_SYNC_DEF CDS_SYNC_INLINE
    void cds_sync_monitor_destroy(cds_sync_monitor_t *mon)
    {
#if   defined(CDS_SYNC_HAS_WINDOWS_CONDVARS)
        /* Windows CONDITION_VARIABLE object do not need to be destroyed. */
        DeleteCriticalSection(&mon->crit);
#elif defined(CDS_SYNC_HAS_POSIX_CONDVARS)
        pthread_cond_destroy(&mon->cv);
        pthread_mutex_destroy(&mon->mtx);
#else
#   error Unsupported compiler/platform
#endif
    }

    CDS_SYNC_DEF CDS_SYNC_INLINE
    void cds_sync_monitor_lock(cds_sync_monitor_t *mon)
    {
#if   defined(CDS_SYNC_HAS_WINDOWS_CONDVARS)
        EnterCriticalSection(&mon->crit);
#elif defined(CDS_SYNC_HAS_POSIX_CONDVARS)
        pthread_mutex_lock(&mon->mtx);
#else
#   error Unsupported compiler/platform
#endif
    }

    CDS_SYNC_DEF CDS_SYNC_INLINE
    void cds_sync_monitor_unlock(cds_sync_monitor_t *mon)
    {
#if   defined(CDS_SYNC_HAS_WINDOWS_CONDVARS)
        LeaveCriticalSection(&mon->crit);
#elif defined(CDS_SYNC_HAS_POSIX_CONDVARS)
        pthread_mutex_unlock(&mon->mtx);
#else
#   error Unsupported compiler/platform
#endif
    }

    CDS_SYNC_DEF CDS_SYNC_INLINE
    void cds_sync_monitor_unlock_and_wait(cds_sync_monitor_t *mon)
    {
#if   defined(CDS_SYNC_HAS_WINDOWS_CONDVARS)
        SleepConditionVariableCS(&mon->cv, &mon->crit, INFINITE);
#elif defined(CDS_SYNC_HAS_POSIX_CONDVARS)
        pthread_cond_wait(&mon->cv, &mon->mtx);
#else
#   error Unsupported compiler/platform
#endif
    }

    CDS_SYNC_DEF CDS_SYNC_INLINE
    void cds_sync_monitor_signal(cds_sync_monitor_t *mon)
    {
#if   defined(CDS_SYNC_HAS_WINDOWS_CONDVARS)
        WakeConditionVariable(&mon->cv);
#elif defined(CDS_SYNC_HAS_POSIX_CONDVARS)
        pthread_cond_signal(&mon->cv);
#else
#   error Unsupported compiler/platform
#endif
    }

    CDS_SYNC_DEF CDS_SYNC_INLINE
    void cds_sync_monitor_broadcast(cds_sync_monitor_t *mon)
    {
#if   defined(CDS_SYNC_HAS_WINDOWS_CONDVARS)
        WakeAllConditionVariable(&mon->cv);
#elif defined(CDS_SYNC_HAS_POSIX_CONDVARS)
        pthread_cond_broadcast(&mon->cv);
#else
#   error Unsupported compiler/platform
#endif
    }

    /**
     * cds_sync_eventcount_t -- an eventcount primitive, which lets you
     * indicate your interest in waiting for an event (with get()), and
     * then only actually waiting if nothing else has signaled in the
     * meantime. This ensures that the waiter only needs to wait if
     * there's legitimately no event in the queue.
     */
    typedef struct
    {
        cds_sync_atomic_s32 count;
        cds_sync_monitor_t mon;
    } cds_sync_eventcount_t;

    /** @brief Initialize an eventcount object. */
    CDS_SYNC_DEF
    int cds_sync_eventcount_init(cds_sync_eventcount_t *ec);

    /** @brief Destroy an eventcount object. */
    CDS_SYNC_DEF
    void cds_sync_eventcount_destroy(cds_sync_eventcount_t *ec);

    /** @brief Return the current event count, in preparation for a
     *         call to wait().  Often called prepare_wait().
     *  @note This is *not* a simple query function; the internal
     *        object state is modified.
     */
    CDS_SYNC_DEF
    cds_sync_s32 cds_sync_eventcount_get(cds_sync_eventcount_t *ec);

    /** @brief Wakes all threads waiting on an eventcount object, and
     *         increment the internal counter.  If no waiters are
     *         registered, this function is a no-op.
     */
    CDS_SYNC_DEF
    void cds_sync_eventcount_signal(cds_sync_eventcount_t *ec);

    /** @brief Compare the provided count to the eventcount object's
     *         internal counter; if they match, put the calling thread
     *         to sleep until the eventcount is signalled.  If the
     *         counts do not match, the caller returns more-or-less
     *         immediately.
     */
    CDS_SYNC_DEF
    void cds_sync_eventcount_wait(cds_sync_eventcount_t *ec, cds_sync_s32 cmp);

    /**
     * cds_sync_monsem_t -- A monitored semaphore is a semaphore that
     * allows two-sided waiting. Like a regular semaphore, a consumer can
     * decrement the semaphore and wait() for the count to be positive
     * again. A producer can increment the semaphore with post(), but can
     * also safely wait for the count to be a certain non-negative value.
     */
    typedef struct
    {
        cds_sync_atomic_s32 state;
        cds_sync_eventcount_t ec;
        cds_sync_fusem_t sem;
    } cds_sync_monsem_t;

    /** @brief Initialize a monitored semaphore to the specified value. */
    CDS_SYNC_DEF
    int cds_sync_monsem_init(cds_sync_monsem_t *ms, cds_sync_s32 count);

    /** @brief Destroy a monitored semaphore object. */
    CDS_SYNC_DEF
    void cds_sync_monsem_destroy(cds_sync_monsem_t *ms);

    /** @brief Waits for the semaphore value to reach a certain
     *         non-negative value.
     *  @note In this implementation, only one thread can call
     *        wait_for_waiters() at a time.
     */
    CDS_SYNC_DEF
    void cds_sync_monsem_wait_for_waiters(cds_sync_monsem_t *ms, cds_sync_s32 waitForCount);

    /** @brief Decrement the semaphore. If the result is 0 or less, put the
     *         caller to sleep.
     */
    CDS_SYNC_DEF
    void cds_sync_monsem_wait(cds_sync_monsem_t *ms);

    /** @brief Increment the semaphore. If the counter was negative, wake a
     *         thread that was previously put to sleep by wait().
     */
    CDS_SYNC_DEF
    void cds_sync_monsem_post(cds_sync_monsem_t *ms);

    /** @brief Increment the semaphore counter N times.
     *  @note  This is functionally equivalent to calling post() N times,
     *         but may be more efficient on some platforms.
     */
    CDS_SYNC_DEF
    void cds_sync_monsem_postn(cds_sync_monsem_t *ms, cds_sync_s32 n);

    /**
     * cds_sync_barrier_t -- A barrier that blocks several threads from
     * progressing until all threads have reached the barrier.
     */
    typedef struct
    {
        cds_sync_fusem_t mutex, semIn, semOut;
        cds_sync_s32 insideCount, threadCount;
    } cds_sync_barrier_t;

    /** @brief Initialize a barrier object for the provided number of
     *         threads.
     */
    CDS_SYNC_DEF
    int cds_sync_barrier_init(cds_sync_barrier_t *barrier, cds_sync_s32 threadCount);

    /** @brief Destroy an existing barrier object. */
    CDS_SYNC_DEF
    void cds_sync_barrier_destroy(cds_sync_barrier_t *barrier);

    /** @brief Enters a barrier's critical section. The 1..N-1th
     *         threads to enter the barrier are put to sleep; the Nth
     *         thread wakes threads 0..N-1.  Must be followed by a
     *         matching call to barrier_exit().
     * @note Any code between barrier_enter() and barrier_exit() is a
     *       type of critical section: no thread will enter it until
     *       all threads have entered, and no thread will exit it
     *       until all threads are ready to exit.
     */
    CDS_SYNC_DEF
    void cds_sync_barrier_enter(cds_sync_barrier_t *barrier);

    /* @brief Exits a barrier's critical section. The 1..N-1th threads
     *        to exit the barrier are put to sleep; the Nth thread
     *        wakes threads 0..N-1.  Must be preceded by a matching
     *        call to barrier_enter().
     * @note Any code between barrier_enter() and barrier_exit() is a
     *       type of critical section: no thread will enter it until
     *       all threads have entered, and no thread will exit it
     *       until all threads are ready to exit.
     */
    CDS_SYNC_DEF
    void cds_sync_barrier_exit(cds_sync_barrier_t *barrier);

#ifdef __cplusplus
}
#endif

#endif /*-------------- end header file ------------------------ */

#if defined(CDS_SYNC_TEST)
#   if !defined(CDS_SYNC_IMPLEMENTATION)
#       define CDS_SYNC_IMPLEMENTATION
#   endif
#endif

#ifdef CDS_SYNC_IMPLEMENTATION

#include <assert.h>
#include <limits.h>
#include <stdint.h>

#define CDS_SYNC_MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define CDS_SYNC_UNUSED(x) ((void)x)

/* Silly Windows, not providing gcc-style atomic intrinsics. This is
 * not an exhaustive implementation; it's only the subset used by
 * cds_sync. Note: not all ATOMIC_* memory orderings have an effect on Windows. */
#if defined(CDS_SYNC_HAS_WINDOWS_ATOMICS)
#   define CDS_SYNC_ATOMIC_RELAXED  0 /* Implies no inter-thread ordering constraints. */
#   define CDS_SYNC_ATOMIC_CONSUME  1 /* Currently just an alias for ACQUIRE. */
#   define CDS_SYNC_ATOMIC_ACQUIRE  2 /* Imposes a happens-before constraint from a release-store. Prevents load-hoisting. */
#   define CDS_SYNC_ATOMIC_RELEASE  3 /* Imposes a happens-before constraint to an acquire-load. Prevents store-sinking. */
#   define CDS_SYNC_ATOMIC_ACQ_REL  4 /* Combines ACQUIRE and RELEASE. Stores won't be sunk; loads won't be hoisted. */
#   define CDS_SYNC_ATOMIC_SEQ_CST  5 /* Enforces total ordering with all other SEQ_CST operations. */
static CDS_SYNC_INLINE cds_sync_s32 cds_sync_atomic_load_s32(cds_sync_s32 *ptr, int memorder) { CDS_SYNC_UNUSED(memorder); return *ptr; }
static CDS_SYNC_INLINE void    cds_sync_atomic_store_s32(cds_sync_s32 *ptr, cds_sync_s32 val, int memorder) { CDS_SYNC_UNUSED(memorder); *ptr = val; }
static CDS_SYNC_INLINE cds_sync_s32 cds_sync_atomic_fetch_add_s32(cds_sync_s32 *ptr, cds_sync_s32 val, int memorder)
{
#   if defined(_M_IX86)
    CDS_SYNC_UNUSED(memorder);
    return _InterlockedExchangeAdd((volatile LONG*)ptr, val);
#   elif defined(_M_X64)
    switch(memorder)
    {
    case CDS_SYNC_ATOMIC_RELAXED:
        return InterlockedExchangeAddNoFence((volatile LONG*)ptr, val);
    case CDS_SYNC_ATOMIC_ACQUIRE:
        return InterlockedExchangeAddAcquire((volatile LONG*)ptr, val);
    case CDS_SYNC_ATOMIC_ACQ_REL:
    case CDS_SYNC_ATOMIC_SEQ_CST:
        return InterlockedExchangeAdd((volatile LONG*)ptr, val);
    default:
        assert(0); /* unsupported memory order */
        return InterlockedExchangeAdd((volatile LONG*)ptr, val);
    }
#   endif
}
static CDS_SYNC_INLINE cds_sync_s32 cds_sync_atomic_fetch_or_s32(cds_sync_s32 *ptr, cds_sync_s32 val, int memorder)
{
#   if defined(_M_IX86)
    CDS_SYNC_UNUSED(memorder);
    return _InterlockedOr((volatile LONG*)ptr, val);
#   elif defined(_M_X64)
    switch(memorder)
    {
    case CDS_SYNC_ATOMIC_RELAXED:
        return InterlockedOrNoFence((volatile LONG*)ptr, val);
    case CDS_SYNC_ATOMIC_ACQUIRE:
        return InterlockedOrAcquire((volatile LONG*)ptr, val);
    case CDS_SYNC_ATOMIC_ACQ_REL:
    case CDS_SYNC_ATOMIC_SEQ_CST:
        return InterlockedOr((volatile LONG*)ptr, val);
    default:
        assert(0); /* unsupported memory order */
        return InterlockedOr((volatile LONG*)ptr, val);
    }
#   endif
}
static CDS_SYNC_INLINE int cds_sync_atomic_compare_exchange_s32(cds_sync_s32 *ptr, cds_sync_s32 *expected, cds_sync_s32 desired,
    int weak, int success_memorder, int failure_memorder)
{
    /* TODO: try to match code generated by gcc/clang intrinsic */
    cds_sync_s32 original;
    int success;
    cds_sync_s32 exp = cds_sync_atomic_load_s32(expected, CDS_SYNC_ATOMIC_SEQ_CST);
    CDS_SYNC_UNUSED(weak);
    CDS_SYNC_UNUSED(failure_memorder);
#   if defined(_M_IX86)
    CDS_SYNC_UNUSED(success_memorder);
    original = _InterlockedCompareExchange((volatile LONG*)ptr, desired, exp);
#   elif defined(_M_X64)
    switch(success_memorder)
    {
    case CDS_SYNC_ATOMIC_RELAXED:
        original = InterlockedCompareExchangeNoFence((volatile LONG*)ptr, desired, exp);
        break;
    case CDS_SYNC_ATOMIC_ACQUIRE:
        original = InterlockedCompareExchangeAcquire((volatile LONG*)ptr, desired, exp);
        break;
    case CDS_SYNC_ATOMIC_ACQ_REL:
    case CDS_SYNC_ATOMIC_SEQ_CST:
        original = InterlockedCompareExchange((volatile LONG*)ptr, desired, exp);
        break;
    default:
        assert(0); /* unsupported memory order */
        original = InterlockedCompareExchange((volatile LONG*)ptr, desired, exp);
        break;
    }
#   endif
    success = (original == exp) ? 1 : 0;
    cds_sync_atomic_store_s32(expected, original, CDS_SYNC_ATOMIC_SEQ_CST);
    return success;
}
#elif defined(CDS_SYNC_HAS_GCC_ATOMICS)
#   define CDS_SYNC_ATOMIC_RELAXED  __ATOMIC_RELAXED
#   define CDS_SYNC_ATOMIC_CONSUME  __ATOMIC_CONSUME
#   define CDS_SYNC_ATOMIC_ACQUIRE  __ATOMIC_ACQUIRE
#   define CDS_SYNC_ATOMIC_RELEASE  __ATOMIC_RELEASE
#   define CDS_SYNC_ATOMIC_ACQ_REL  __ATOMIC_ACQ_REL
#   define CDS_SYNC_ATOMIC_SEQ_CST  __ATOMIC_SEQ_CST
static CDS_SYNC_INLINE cds_sync_s32 cds_sync_atomic_load_s32(cds_sync_s32 *ptr, int memorder) { return __atomic_load_n(ptr, memorder); }
static CDS_SYNC_INLINE void    cds_sync_atomic_store_s32(cds_sync_s32 *ptr, cds_sync_s32 val, int memorder) { return __atomic_store_n(ptr, val, memorder); }
static CDS_SYNC_INLINE cds_sync_s32 cds_sync_atomic_fetch_add_s32(cds_sync_s32 *ptr, cds_sync_s32 val, int memorder) { return __atomic_fetch_add(ptr, val, memorder); }
static CDS_SYNC_INLINE cds_sync_s32 cds_sync_atomic_fetch_or_s32(cds_sync_s32 *ptr, cds_sync_s32 val, int memorder) { return __atomic_fetch_or(ptr, val, memorder); }
static CDS_SYNC_INLINE int cds_sync_atomic_compare_exchange_s32(cds_sync_s32 *ptr, cds_sync_s32 *expected, cds_sync_s32 desired,
    int weak, int success_memorder, int failure_memorder)
{
    return __atomic_compare_exchange_n(ptr, expected, desired, weak, success_memorder, failure_memorder);
}
#else
#   error Unsupported compiler/platform
#endif

/* cds_sync_fusem_t */
int cds_sync_fusem_init(cds_sync_fusem_t *sem, cds_sync_s32 n)
{
#if   defined(CDS_SYNC_HAS_WINDOWS_SEMAPHORES)
    sem->count.n = n;
    sem->handle = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
    if (sem->handle == NULL)
        return GetLastError();
    return 0;
#elif defined(CDS_SYNC_HAS_GCD_SEMAPHORES)
    sem->sem = dispatch_semaphore_create(0);
    sem->count.n = n;
    return (sem->sem != NULL) ? 0 : -1;
#elif defined(CDS_SYNC_HAS_POSIX_SEMAPHORES)
    int err = sem_init(&sem->sem, 0, 0);
    sem->count.n = n;
    return err;
#else
#   error Unsupported compiler/platform
#endif
}

void cds_sync_fusem_destroy(cds_sync_fusem_t *sem)
{
#if defined(CDS_SYNC_HAS_WINDOWS_SEMAPHORES)
    CloseHandle(sem->handle);
#elif defined(CDS_SYNC_HAS_GCD_SEMAPHORES)
    dispatch_release(sem->sem);
#elif defined(CDS_SYNC_HAS_POSIX_SEMAPHORES)
    sem_destroy(&sem->sem);
#else
#   error Unsupported compiler/platform
#endif
}

static CDS_SYNC_INLINE int cds_sync_fusem_trywait(cds_sync_fusem_t *sem)
{
    cds_sync_s32 count = cds_sync_atomic_load_s32(&sem->count.n, CDS_SYNC_ATOMIC_ACQUIRE);
    while(count > 0)
    {
        if (cds_sync_atomic_compare_exchange_s32(&sem->count.n, &count, count-1,
                1, CDS_SYNC_ATOMIC_ACQ_REL, CDS_SYNC_ATOMIC_RELAXED))
        {
            return 1;
        }
        /* otherwise, count was reloaded. backoff here is optional. */
    }
    return 0;
}

static CDS_SYNC_INLINE void cds_sync_fusem_wait_no_spin(cds_sync_fusem_t *sem)
{
    if (cds_sync_atomic_fetch_add_s32(&sem->count.n, -1, CDS_SYNC_ATOMIC_ACQ_REL) < 1)
    {
#if defined(CDS_SYNC_HAS_WINDOWS_SEMAPHORES)
        WaitForSingleObject(sem->handle, INFINITE);
#elif defined(CDS_SYNC_HAS_GCD_SEMAPHORES)
        dispatch_semaphore_wait(sem->sem, DISPATCH_TIME_FOREVER);
#elif defined(CDS_SYNC_HAS_POSIX_SEMAPHORES)
        sem_wait(&sem->sem);
#else
#   error Unsupported compiler/platform
#endif
    }
}

void cds_sync_fusem_wait(cds_sync_fusem_t *sem)
{
    int spin_count = 1;
    while(spin_count--)
    {
        if (cds_sync_fusem_trywait(sem) )
        {
            return;
        }
    }
    cds_sync_fusem_wait_no_spin(sem);
}

void cds_sync_fusem_post(cds_sync_fusem_t *sem)
{
    if (cds_sync_atomic_fetch_add_s32(&sem->count.n, 1, CDS_SYNC_ATOMIC_ACQ_REL) < 0)
    {
#if defined(CDS_SYNC_HAS_WINDOWS_SEMAPHORES)
        ReleaseSemaphore(sem->handle, 1, 0);
#elif defined(CDS_SYNC_HAS_GCD_SEMAPHORES)
        dispatch_semaphore_signal(sem->sem);
#elif defined(CDS_SYNC_HAS_POSIX_SEMAPHORES)
        sem_post(&sem->sem);
#else
#   error Unsupported compiler/platform
#endif
    }
}

void cds_sync_fusem_postn(cds_sync_fusem_t *sem, cds_sync_s32 n)
{
    cds_sync_s32 oldCount = cds_sync_atomic_fetch_add_s32(&sem->count.n, n, CDS_SYNC_ATOMIC_ACQ_REL);
    if (oldCount < 0)
    {
        cds_sync_s32 numWaiters = -oldCount;
        cds_sync_s32 numToWake = CDS_SYNC_MIN(numWaiters, n);
#if defined(CDS_SYNC_HAS_WINDOWS_SEMAPHORES)
        ReleaseSemaphore(sem->handle, numToWake, 0);
#elif defined(CDS_SYNC_HAS_GCD_SEMAPHORES)
        /* wakeN would be better than a loop here, but hey */
        cds_sync_s32 iWake;
        for(iWake=0; iWake<numToWake; iWake += 1)
        {
            dispatch_semaphore_signal(sem->sem);
        }
#elif defined(CDS_SYNC_HAS_POSIX_SEMAPHORES)
        /* wakeN would be better than a loop here, but hey */
        cds_sync_s32 iWake;
        for(iWake=0; iWake<numToWake; iWake += 1)
        {
            sem_post(&sem->sem);
        }
#else
#   error Unsupported compiler/platform
#endif
    }
}

cds_sync_s32 cds_sync_fusem_getvalue(cds_sync_fusem_t *sem)
{
    return cds_sync_atomic_load_s32(&sem->count.n, CDS_SYNC_ATOMIC_SEQ_CST);
}


/* cds_sync_eventcount_t */
int cds_sync_eventcount_init(cds_sync_eventcount_t *ec)
{
    cds_sync_monitor_init(&ec->mon);
    cds_sync_atomic_store_s32(&ec->count.n, 0, CDS_SYNC_ATOMIC_RELAXED);
    return 0;
}
void cds_sync_eventcount_destroy(cds_sync_eventcount_t *ec)
{
    cds_sync_monitor_destroy(&ec->mon);
}
cds_sync_s32 cds_sync_eventcount_get(cds_sync_eventcount_t *ec)
{
    return cds_sync_atomic_fetch_or_s32(&ec->count.n, 1, CDS_SYNC_ATOMIC_ACQUIRE);
}
void cds_sync_eventcount_signal(cds_sync_eventcount_t *ec)
{
    cds_sync_s32 key = cds_sync_atomic_fetch_add_s32(&ec->count.n, 0, CDS_SYNC_ATOMIC_SEQ_CST);
    if (key & 1)
    {
        cds_sync_monitor_lock(&ec->mon);
        while (!cds_sync_atomic_compare_exchange_s32(&ec->count.n, &key, (key+2) & ~1,
                1, CDS_SYNC_ATOMIC_SEQ_CST, CDS_SYNC_ATOMIC_SEQ_CST))
        {
            /* spin */
        }
        cds_sync_monitor_unlock(&ec->mon);
        cds_sync_monitor_broadcast(&ec->mon);
    }
}
void cds_sync_eventcount_wait(cds_sync_eventcount_t *ec, cds_sync_s32 cmp)
{
    cds_sync_monitor_lock(&ec->mon);
    if ((cds_sync_atomic_load_s32(&ec->count.n, CDS_SYNC_ATOMIC_SEQ_CST) & ~1) == (cmp & ~1))
    {
        cds_sync_monitor_unlock_and_wait(&ec->mon);
    }
    cds_sync_monitor_unlock(&ec->mon);
}


/* cds_sync_monsem_t */
#define CDS_SYNC_MONSEM_COUNT_SHIFT 8
#define CDS_SYNC_MONSEM_COUNT_MASK  0xFFFFFF00UL
#define CDS_SYNC_MONSEM_COUNT_MAX   ((cds_sync_u32)(CDS_SYNC_MONSEM_COUNT_MASK) >> (CDS_SYNC_MONSEM_COUNT_SHIFT))
#define CDS_SYNC_MONSEM_WAIT_FOR_SHIFT 0
#define CDS_SYNC_MONSEM_WAIT_FOR_MASK 0xFF
#define CDS_SYNC_MONSEM_WAIT_FOR_MAX ((CDS_SYNC_MONSEM_WAIT_FOR_MASK) >> (CDS_SYNC_MONSEM_WAIT_FOR_SHIFT))

int cds_sync_monsem_init(cds_sync_monsem_t *ms, cds_sync_s32 count)
{
    assert(count >= 0);
    cds_sync_fusem_init(&ms->sem, 0);
    cds_sync_eventcount_init(&ms->ec);
    ms->state.n = count << CDS_SYNC_MONSEM_COUNT_SHIFT;
    return 0;
}
void cds_sync_monsem_destroy(cds_sync_monsem_t *ms)
{
    cds_sync_fusem_destroy(&ms->sem);
    cds_sync_eventcount_destroy(&ms->ec);
}

void cds_sync_monsem_wait_for_waiters(cds_sync_monsem_t *ms, cds_sync_s32 waitForCount)
{
    cds_sync_s32 state;
    assert( waitForCount > 0 && waitForCount < CDS_SYNC_MONSEM_WAIT_FOR_MAX );
    state = cds_sync_atomic_load_s32(&ms->state.n, CDS_SYNC_ATOMIC_ACQUIRE);
    for(;;)
    {
        cds_sync_s32 newState, ec;
        cds_sync_s32 curCount = state >> CDS_SYNC_MONSEM_COUNT_SHIFT;
        if ( -curCount == waitForCount )
        {
            break;
        }
        newState = (curCount     << CDS_SYNC_MONSEM_COUNT_SHIFT)
            | (waitForCount << CDS_SYNC_MONSEM_WAIT_FOR_SHIFT);
        ec = cds_sync_eventcount_get(&ms->ec);
        if (!cds_sync_atomic_compare_exchange_s32(&ms->state.n, &state, newState,
                0, CDS_SYNC_ATOMIC_ACQ_REL, CDS_SYNC_ATOMIC_ACQUIRE))
        {
            continue; /* retry; state was reloaded */
        }
        cds_sync_eventcount_wait(&ms->ec, ec);
        state = cds_sync_atomic_load_s32(&ms->state.n, CDS_SYNC_ATOMIC_ACQUIRE);
    }
    for(;;)
    {
        cds_sync_s32 newState = state & CDS_SYNC_MONSEM_COUNT_MASK;
        if (state == newState)
        {
            return; /* nothing to do */
        }
        if (cds_sync_atomic_compare_exchange_s32(&ms->state.n, &state, newState,
                0, CDS_SYNC_ATOMIC_ACQ_REL, CDS_SYNC_ATOMIC_ACQUIRE))
        {
            return; /* updated successfully */
        }
        /* retry; state was reloaded */
    }
}

static CDS_SYNC_INLINE void cds_sync_monsem_wait_no_spin(cds_sync_monsem_t *ms)
{
    /* cds_sync_s32 prevState = ((count-1)<<COUNT_SHIFT) | (state & WAIT_FOR_MASK); */
    cds_sync_s32 prevState = cds_sync_atomic_fetch_add_s32(&ms->state.n, (-1)<<CDS_SYNC_MONSEM_COUNT_SHIFT,
        CDS_SYNC_ATOMIC_ACQ_REL);

    cds_sync_s32 prevCount = prevState >> CDS_SYNC_MONSEM_COUNT_SHIFT; /* arithmetic shift is intentional */
    if (prevCount <= 0)
    {
        cds_sync_s32 waiters = (-prevCount) + 1;
        cds_sync_s32 waitFor = prevState & CDS_SYNC_MONSEM_WAIT_FOR_MASK;
        assert(waiters >= 1);
        if (waiters == waitFor)
        {
            assert(waitFor >= 1);
            cds_sync_eventcount_signal(&ms->ec);
        }
        cds_sync_fusem_wait(&ms->sem);
    }
}

static CDS_SYNC_INLINE int cds_sync_monsem_try_wait(cds_sync_monsem_t *ms)
{
    /* See if we can decrement the count before preparing the wait. */
    cds_sync_s32 state = cds_sync_atomic_load_s32(&ms->state.n, CDS_SYNC_ATOMIC_ACQUIRE);
    for(;;)
    {
        cds_sync_s32 newState;
        if (state < (1<<CDS_SYNC_MONSEM_COUNT_SHIFT) )
        {
            return 0;
        }
        /* newState = ((count-1)<<COUNT_SHIFT) | (state & WAIT_FOR_MASK); */
        newState = state - (1<<CDS_SYNC_MONSEM_COUNT_SHIFT);
        assert( (newState >> CDS_SYNC_MONSEM_COUNT_SHIFT) >= 0 );
        if (cds_sync_atomic_compare_exchange_s32(&ms->state.n, &state, newState,
                0, CDS_SYNC_ATOMIC_ACQ_REL, CDS_SYNC_ATOMIC_ACQUIRE))
        {
            return 1;
        }
        /* state was reloaded; try again, with optional backoff. */
    }
}

#if 0
static CDS_SYNC_INLINE int cds_sync_monsem_try_wait_all(cds_sync_monsem_t *ms)
{
    cds_sync_s32 state = cds_sync_atomic_load_s32(&ms->state, CDS_SYNC_ATOMIC_ACQUIRE);
    for(;;)
    {
        cds_sync_s32 newState;
        cds_sync_s32 count = state >> CDS_SYNC_MONSEM_COUNT_SHIFT;
        if (count <= 0)
        {
            return 0;
        }
        /* zero out the count */
        newState = state & CDS_SYNC_MONSEM_WAIT_FOR_MASK;
        if (cds_sync_atomic_compare_exchange_s32(&ms->state, &state, newState,
                0, CDS_SYNC_ATOMIC_ACQ_REL, CDS_SYNC_ATOMIC_ACQUIRE))
        {
            return count;
        }
        /* state was reloaded; try again, with optional backoff. */
    }
}
#endif

void cds_sync_monsem_wait(cds_sync_monsem_t *ms)
{
    int spinCount = 1;
    while(spinCount -= 1)
    {
        if (cds_sync_monsem_try_wait(ms))
        {
            return;
        }
    }
    cds_sync_monsem_wait_no_spin(ms);
}

void cds_sync_monsem_post(cds_sync_monsem_t *ms)
{
    const cds_sync_s32 inc = 1;
    cds_sync_s32 prev = cds_sync_atomic_fetch_add_s32(&ms->state.n, inc<<CDS_SYNC_MONSEM_COUNT_SHIFT,
        CDS_SYNC_ATOMIC_ACQ_REL);
    cds_sync_s32 count = (prev >> CDS_SYNC_MONSEM_COUNT_SHIFT);
    assert(count < 0  || ( (unsigned int)count < (CDS_SYNC_MONSEM_COUNT_MAX-2) ));
    if (count < 0)
    {
        cds_sync_fusem_post(&ms->sem);
    }
}

void cds_sync_monsem_postn(cds_sync_monsem_t *ms, cds_sync_s32 n)
{
    cds_sync_s32 i;
    assert(n > 0);
    for(i=0; i<n; i+=1)
    {
        cds_sync_monsem_post(ms);
    }
}


/* cds_sync_barrier_t */
int cds_sync_barrier_init(cds_sync_barrier_t *barrier, cds_sync_s32 threadCount)
{
    assert(threadCount > 0);
    barrier->insideCount = 0;
    barrier->threadCount = threadCount;
    cds_sync_fusem_init(&barrier->mutex, 1);
    cds_sync_fusem_init(&barrier->semIn, 0);
    cds_sync_fusem_init(&barrier->semOut, 0);
    return 0;
}

void cds_sync_barrier_destroy(cds_sync_barrier_t *barrier)
{
    cds_sync_fusem_destroy(&barrier->mutex);
    cds_sync_fusem_destroy(&barrier->semIn);
    cds_sync_fusem_destroy(&barrier->semOut);
}

void cds_sync_barrier_enter(cds_sync_barrier_t *barrier)
{
    cds_sync_fusem_wait(&barrier->mutex);
    barrier->insideCount += 1;
    if (barrier->insideCount == barrier->threadCount)
    {
        cds_sync_fusem_postn(&barrier->semIn, barrier->threadCount);
    }
    cds_sync_fusem_post(&barrier->mutex);
    cds_sync_fusem_wait(&barrier->semIn);
}

void cds_sync_barrier_exit(cds_sync_barrier_t *barrier)
{
    cds_sync_fusem_wait(&barrier->mutex);
    barrier->insideCount -= 1;
    if (barrier->insideCount == 0)
    {
        cds_sync_fusem_postn(&barrier->semOut, barrier->threadCount);
    }
    cds_sync_fusem_post(&barrier->mutex);
    cds_sync_fusem_wait(&barrier->semOut);
}


#endif /*------------- end implementation section ---------------*/

#if defined(CDS_SYNC_TEST)

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Minimal thread management wrappers */
#if defined(CDS_SYNC_HAS_WINDOWS_THREADS)
typedef HANDLE cds_sync_thread_t;
typedef DWORD cds_sync_threadproc_return_t;
#   define CDS_SYNC_THREADPROC WINAPI
static CDS_SYNC_INLINE void cds_sync_thread_create(cds_sync_thread_t *pThread, LPTHREAD_START_ROUTINE startProc, void *args) { *pThread = CreateThread(NULL,0,startProc,args,0,NULL); }
static CDS_SYNC_INLINE void cds_sync_thread_join(cds_sync_thread_t thread) { WaitForSingleObject(thread, INFINITE); CloseHandle(thread); }
static CDS_SYNC_INLINE int cds_sync_thread_id(void) { return (int)GetCurrentThreadId(); }
#elif defined(CDS_SYNC_HAS_POSIX_THREADS)
typedef pthread_t cds_sync_thread_t;
typedef void* cds_sync_threadproc_return_t;
#   define CDS_SYNC_THREADPROC
static CDS_SYNC_INLINE void cds_sync_thread_create(cds_sync_thread_t *pThread, void *(*startProc)(void*), void *args) { pthread_create(pThread,NULL,startProc,args); }
static CDS_SYNC_INLINE void cds_sync_thread_join(cds_sync_thread_t thread) { pthread_join(thread, NULL); }
static CDS_SYNC_INLINE int cds_sync_thread_id(void) { return (int)pthread_self(); }
#endif

#if defined(CDS_SYNC_HAS_WINDOWS_YIELD)
static CDS_SYNC_INLINE void cds_sync_thread_yield(void) { YieldProcessor(); }
#elif defined(CDS_SYNC_HAS_SCHED_YIELD)
#    include <sched.h>
static CDS_SYNC_INLINE void cds_sync_thread_yield(void) { sched_yield(); }
#elif defined(CDS_SYNC_HAS_PTHREAD_YIELD_NP)
static CDS_SYNC_INLINE void cds_sync_thread_yield(void) { pthread_yield_np(); }
#else
#   error Unsupported compiler/platform
#endif

static cds_sync_s32 g_errorCount = 0;

#define CDS_TEST_DANCER_NUM_ROUNDS 1000
typedef struct
{
    struct
    {
        cds_sync_s32 leaderId;
        cds_sync_s32 followerId;
    } rounds[CDS_TEST_DANCER_NUM_ROUNDS];
    cds_sync_fusem_t queueL, queueF, mutexL, mutexF, rendezvous;
    cds_sync_s32 roundIndex;
} cds_test_dancer_args_t;
static cds_sync_threadproc_return_t CDS_SYNC_THREADPROC testDancerLeader(void *voidArgs)
{
    cds_test_dancer_args_t *args = (cds_test_dancer_args_t*)voidArgs;
    int threadId = cds_sync_thread_id();
    cds_sync_s32 lastRoundIndex = 0;
    cds_sync_s32 zero = 0;
    for(;;)
    {
        zero = 0;
        cds_sync_fusem_wait(&args->mutexL);
        cds_sync_fusem_post(&args->queueL);
        cds_sync_fusem_wait(&args->queueF);
        /* critical section */
        if (args->roundIndex < CDS_TEST_DANCER_NUM_ROUNDS)
        {
            zero = cds_sync_atomic_fetch_add_s32(&args->rounds[args->roundIndex].leaderId, threadId,
                CDS_SYNC_ATOMIC_SEQ_CST);
        }
        cds_sync_fusem_wait(&args->rendezvous);
        lastRoundIndex = cds_sync_atomic_fetch_add_s32(&args->roundIndex, 1, CDS_SYNC_ATOMIC_SEQ_CST);
        /* end critical section */
        cds_sync_fusem_post(&args->mutexL);
        if (0 != zero)
        {
            printf("ERROR: double-write to rounds[%d].leaderId (expected 0, found %d)\n",
                lastRoundIndex, zero);
            cds_sync_atomic_fetch_add_s32(&g_errorCount, 1, CDS_SYNC_ATOMIC_SEQ_CST);
        }
        if (lastRoundIndex+1 >= CDS_TEST_DANCER_NUM_ROUNDS)
        {
            break;
        }
    }
    return (cds_sync_threadproc_return_t)NULL;
}

static cds_sync_threadproc_return_t CDS_SYNC_THREADPROC testDancerFollower(void *voidArgs)
{
    cds_test_dancer_args_t *args = (cds_test_dancer_args_t*)voidArgs;
    int threadId = cds_sync_thread_id();
    cds_sync_s32 lastRoundIndex = 0;
    cds_sync_s32 zero = 0;
    for(;;)
    {
        zero = 0;
        cds_sync_fusem_wait(&args->mutexF);
        cds_sync_fusem_post(&args->queueF);
        cds_sync_fusem_wait(&args->queueL);
        /* critical section */
        lastRoundIndex = cds_sync_atomic_load_s32(&args->roundIndex, CDS_SYNC_ATOMIC_SEQ_CST);
        if (args->roundIndex < CDS_TEST_DANCER_NUM_ROUNDS)
        {
            zero = cds_sync_atomic_fetch_add_s32(&args->rounds[args->roundIndex].followerId, threadId,
                CDS_SYNC_ATOMIC_SEQ_CST);
        }
        cds_sync_fusem_post(&args->rendezvous);
        /* end critical section */
        cds_sync_fusem_post(&args->mutexF);
        if (0 != zero)
        {
            printf("ERROR: double-write to rounds[%d].followererId (expected 0, found %d)\n",
                lastRoundIndex, zero);
            cds_sync_atomic_fetch_add_s32(&g_errorCount, 1, CDS_SYNC_ATOMIC_SEQ_CST);
        }
        if (lastRoundIndex+1 >= CDS_TEST_DANCER_NUM_ROUNDS)
        {
            break;
        }
    }
    return (cds_sync_threadproc_return_t)NULL;
}

static void testDancers(void)
{
    /*
     * Use semaphores to implement a pair of queues: one for leaders,
     * one for followers. The queue protects a critical section, which
     * should be entered by exactly one of each type at a time.
     */
    cds_test_dancer_args_t dancerArgs;
    cds_sync_thread_t *leaderThreads = NULL, *followerThreads = NULL;
    const int kNumLeaders = 8, kNumFollowers = 8;
    int iLeader=0, iFollower=0, iRound=0;

    dancerArgs.roundIndex = 0;
    memset(dancerArgs.rounds, 0, sizeof(dancerArgs.rounds));
    cds_sync_fusem_init(&dancerArgs.queueL, 0);
    cds_sync_fusem_init(&dancerArgs.queueF, 0);
    cds_sync_fusem_init(&dancerArgs.mutexL, 1);
    cds_sync_fusem_init(&dancerArgs.mutexF, 1);
    cds_sync_fusem_init(&dancerArgs.rendezvous, 0);

    leaderThreads = (cds_sync_thread_t*)malloc(kNumLeaders*sizeof(cds_sync_thread_t));
    for(iLeader=0; iLeader<kNumLeaders; ++iLeader)
    {
        cds_sync_thread_create(&leaderThreads[iLeader], testDancerLeader, &dancerArgs);
    }
    followerThreads = (cds_sync_thread_t*)malloc(kNumFollowers*sizeof(cds_sync_thread_t));
    for(iFollower=0; iFollower<kNumFollowers; ++iFollower)
    {
        cds_sync_thread_create(&followerThreads[iFollower], testDancerFollower, &dancerArgs);
    }

    for(iLeader=0; iLeader<kNumLeaders; ++iLeader)
    {
        cds_sync_thread_join(leaderThreads[iLeader]);
    }
    for(iFollower=0; iFollower<kNumFollowers; ++iFollower)
    {
        cds_sync_thread_join(followerThreads[iFollower]);
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

    cds_sync_fusem_destroy(&dancerArgs.queueL);
    cds_sync_fusem_destroy(&dancerArgs.queueF);
    cds_sync_fusem_destroy(&dancerArgs.mutexL);
    cds_sync_fusem_destroy(&dancerArgs.mutexF);
    cds_sync_fusem_destroy(&dancerArgs.rendezvous);
    free(leaderThreads);
    free(followerThreads);
}

#define CDS_TEST_QUEUE_LENGTH 100
/* #define CDS_TEST_QUEUE_ENABLE_BROKEN_MODE */
typedef struct cds_test_queue_args_t
{
    cds_sync_futex_t mtx; /* protects queue; too lazy to write a lockless */
#if defined(CDS_TEST_QUEUE_ENABLE_BROKEN_MODE)
    cds_sync_monitor_t mon;
#else
    cds_sync_eventcount_t ec;
#endif
    cds_sync_s32 input[CDS_TEST_QUEUE_LENGTH];
    cds_sync_s32 output[CDS_TEST_QUEUE_LENGTH];
    cds_sync_s32 readIndex, writeIndex;
} cds_test_queue_args_t;
static cds_sync_threadproc_return_t CDS_SYNC_THREADPROC testQueuePopper(void *voidArgs)
{
    cds_test_queue_args_t *args = (cds_test_queue_args_t*)voidArgs;
    cds_sync_s32 n = 0;
    while(n >= 0 && args->readIndex < CDS_TEST_QUEUE_LENGTH)
    {
        cds_sync_s32 count = 0;
        int gotOne = 0;
        cds_sync_futex_lock(&args->mtx);
        if (args->readIndex < args->writeIndex)
        {
            n = args->input[args->readIndex++];
            gotOne = 1;
        }
        cds_sync_futex_unlock(&args->mtx);
        if (gotOne)
        {
            if (args->output[abs(n)] != 0)
                printf("0x%08X: output[%3d] = %3d (expected 0)\n", cds_sync_thread_id(), abs(n), args->output[abs(n)]);
            args->output[abs(n)] += n;
            continue;
        }
#if defined(CDS_TEST_QUEUE_ENABLE_BROKEN_MODE)
        /* A pure monitor-based approach suffers from lost wakeups; if the monitor_signal()
           occurs between the test for an empty queue and the call to
           monitor_unlock_and_wait(), we won't wake up! */

        /* A yield here isn't necessary, but helps to encourage race conditions
           (if any) to manifest. */
        cds_sync_thread_yield();
        cds_sync_monitor_lock(&args->mon);
        cds_sync_monitor_unlock_and_wait(&args->mon);
        cds_sync_monitor_unlock(&args->mon);
#else
        /* queue was empty; get() the EC and try again */
        count = cds_sync_eventcount_get(&args->ec);
        /* A yield here isn't necessary, but helps to encourage race conditions
         * (if any) to manifest. */
        cds_sync_thread_yield();
        cds_sync_futex_lock(&args->mtx);
        if (args->readIndex < args->writeIndex)
        {
            n = args->input[args->readIndex++];
            gotOne = 1;
        }
        cds_sync_futex_unlock(&args->mtx);
        if (gotOne)
        {
            /* TODO: cancel EC get()? */
            if (args->output[abs(n)] != 0)
                printf("0x%08X: output[%3d] = %3d (expected 0)\n", cds_sync_thread_id(), abs(n), args->output[abs(n)]);
            args->output[abs(n)] += n;
            continue;
        }
        /* queue still empty; wait until it has something available. */
        cds_sync_eventcount_wait(&args->ec, count);
#endif

    }
    return 0;
}
static void testEventcount(void)
{
    cds_test_queue_args_t args;
    cds_sync_s32 iEntry, iPopper;
    cds_sync_thread_t *popperThreads = NULL;
    const int kNumPoppers = 8;

    cds_sync_futex_init(&args.mtx);
#if defined(CDS_TEST_QUEUE_ENABLE_BROKEN_MODE)
    cds_sync_monitor_init(&args.mon);
#else
    cds_sync_eventcount_init(&args.ec);
#endif
    for(iEntry=0; iEntry<CDS_TEST_QUEUE_LENGTH; ++iEntry)
    {
        args.input[iEntry]  = (iEntry+kNumPoppers < CDS_TEST_QUEUE_LENGTH) ? iEntry : -iEntry;
        args.output[iEntry] = 0;
    }
    args.readIndex = 0;
    args.writeIndex = 0;

    popperThreads = (cds_sync_thread_t*)malloc(kNumPoppers*sizeof(cds_sync_thread_t));
    for(iPopper=0; iPopper<kNumPoppers; ++iPopper)
    {
        cds_sync_thread_create(&popperThreads[iPopper], testQueuePopper, &args);
    }

    iEntry=0;
    for(;;)
    {
        cds_sync_s32 numPushes = rand() % 4 + 1;
        if (args.writeIndex + numPushes > CDS_TEST_QUEUE_LENGTH)
            numPushes = CDS_TEST_QUEUE_LENGTH - args.writeIndex;
        cds_sync_atomic_fetch_add_s32(&args.writeIndex, numPushes, CDS_SYNC_ATOMIC_SEQ_CST);
#if defined(CDS_TEST_QUEUE_ENABLE_BROKEN_MODE)
        cds_sync_monitor_broadcast(&args.mon);
#else
        cds_sync_eventcount_signal(&args.ec);
#endif
        if (args.writeIndex == CDS_TEST_QUEUE_LENGTH)
        {
            break;
        }
        cds_sync_thread_yield();
    }

    for(iPopper=0; iPopper<kNumPoppers; ++iPopper)
    {
        cds_sync_thread_join(popperThreads[iPopper]);
    }
    for(iEntry=0; iEntry<CDS_TEST_QUEUE_LENGTH; ++iEntry)
    {
        if (abs(args.output[iEntry]) != iEntry)
        {
            printf("ERROR: double-write to output[%d]; expected %d, found %d\n", iEntry, iEntry, args.output[iEntry]);
            ++g_errorCount;
        }
    }

    cds_sync_futex_destroy(&args.mtx);
#if defined(CDS_TEST_QUEUE_ENABLE_BROKEN_MODE)
    cds_sync_monitor_destroy(&args.mon);
#else
    cds_sync_eventcount_destroy(&args.ec);
#endif
}

#if   defined(CDS_SYNC_PLATFORM_WINDOWS)
static cds_sync_s32 cds_sync_cache_line_size(void)
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION *procInfos = NULL;
    DWORD procInfoBufferSize = 0;
    size_t cacheLineSizeL1 = 0;
    unsigned int iProc=0;
    GetLogicalProcessorInformation(0, &procInfoBufferSize);
    procInfos = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*)malloc(procInfoBufferSize);
    GetLogicalProcessorInformation(&procInfos[0], &procInfoBufferSize);
    for(iProc=0; iProc<procInfoBufferSize/sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++iProc)
    {
        if (procInfos[iProc].Relationship == RelationCache &&
            procInfos[iProc].Cache.Level == 1)
        {
            cacheLineSizeL1 = procInfos[iProc].Cache.LineSize;
            break;
        }
    }
    assert(cacheLineSizeL1 > 0);
    free(procInfos);
    return (cds_sync_s32)cacheLineSizeL1;
}
#elif defined(CDS_SYNC_PLATFORM_OSX)
#   include <sys/types.h>
#   include <sys/sysctl.h>
static cds_sync_s32 cds_sync_cache_line_size(void)
{
    size_t cacheLineSizeL1 = 0;
    size_t sizeofLineSize = sizeof(cacheLineSizeL1);
    sysctlbyname("hw.cachelinesize", &cacheLineSizeL1, &sizeofLineSize, 0, 0);
    return (cds_sync_s32)cacheLineSizeL1;
}
#elif defined(CDS_SYNC_PLATFORM_POSIX)
#   include <sys/sysctl.h>
static cds_sync_s32 cds_sync_cache_line_size(void)
{
    return (cds_sync_s32)sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
}
#endif

static CDS_SYNC_INLINE void cds_sync_sleep_ms(cds_sync_s32 ms)
{
    if (ms < 0) return;
    {
#if defined(CDS_SYNC_HAS_WINDOWS_THREADS)
        Sleep(ms);
#elif defined(CDS_SYNC_HAS_POSIX_THREADS)
        struct timespec req = {}, rem = {};
        req.tv_sec = ms / 1000;
        req.tv_nsec = (ms % 1000) * 1000000;
        nanosleep(&req, &rem);
#else
#   error Unsupported platform
#endif
    }
}

typedef enum
{
    kCdsSyncTestDancers    = 0,
    kCdsSyncTestEventcount = 1,

    kNumCdsSyncTests
} CdsSyncTest;
static const char *g_testNames[kNumCdsSyncTests] =
{
    "testDancers",
    "testEventCount",
};
static cds_sync_s32 g_testCount;
static CdsSyncTest g_currentTest;
static cds_sync_threadproc_return_t CDS_SYNC_THREADPROC watchdogFunc(void *voidArgs)
{
    cds_sync_s32 lastTestCount = 0;
    const cds_sync_s32 intervalMs = 5000;
    CDS_SYNC_UNUSED(voidArgs);
    for(;;)
    {
        cds_sync_sleep_ms(intervalMs);
        if (lastTestCount == g_testCount)
        {
            printf("ERROR: %s has been running for >%.1f seconds; check for deadlock.\n",
                g_testNames[g_currentTest], (float)intervalMs / 1000.0f);
            break;
        }
        lastTestCount = g_testCount;
        printf("%9d tests complete; %9d errors detected.\n", lastTestCount, g_errorCount);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    cds_sync_s32 cacheLineSizeL1 = cds_sync_cache_line_size();
    unsigned int seed = (unsigned int)time(NULL);
    cds_sync_thread_t watchdogThread;
    CDS_SYNC_UNUSED(argc);
    CDS_SYNC_UNUSED(argv);

    printf("L1 line size: %d\n", (int)cacheLineSizeL1);
    if (CDS_SYNC_L1_CACHE_LINE_SIZE < cacheLineSizeL1)
    {
        printf("WARNING: L1 cache size on this system is %d bytes;\nCDS_SYNC_L1_CACHE_LINE_SIZE should be edited to match!\n",
            (int)cacheLineSizeL1);
    }

    printf("random seed: 0x%08X\n", seed);
    srand(seed);

    cds_sync_thread_create(&watchdogThread, watchdogFunc, NULL);

    for(;;)
    {
        g_currentTest = kCdsSyncTestDancers;
        testDancers();
        g_testCount += 1;

        g_currentTest = kCdsSyncTestEventcount;
        testEventcount();
        g_testCount += 1;
    }
}
#endif /*------------------- send self-test section ------------*/
