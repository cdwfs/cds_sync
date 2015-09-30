cds_sync
========

This
[single header file](https://github.com/nothings/stb/blob/master/docs/other_libs.md)
C90 library provides a portable collection of synchronization
primitives for use in multithreaded programming. The following
primitives are provided:

- `cds_sync_futex_t` -- A [futex](https://en.wikipedia.org/wiki/Futex)
  (fast userspace [mutex](https://en.wikipedia.org/wiki/Mutex)),
  guaranteed to stay in userspace code unless a thread must be put to
  sleep or awakened.
- `cds_sync_fusem_t` -- A fast userspace
  [semaphore](https://en.wikipedia.org/wiki/Semaphore_%28programming%29),
  guaranteed to stay in userspace code unless a thread must be put to
  sleep or awakened.
- `cds_sync_monitor_t` -- A
  [monitor](https://en.wikipedia.org/wiki/Monitor_%28synchronization%29),
  which bundles a condition variable and its associated mutex.
- `cds_sync_eventcount_t` -- An
  [event count](http://cbloomrants.blogspot.com/2011/07/07-08-11-who-ordered-event-count.html)
  which lets callers safely avoid waiting unless there's actually no
  work to do.
- `cds_sync_monsem_t` -- A
  [monitored semaphore](http://cbloomrants.blogspot.com/2015/05/05-16-15-threading-primitive-monitored.html),
  which builds on the basic semaphore by allowing a master thread to
  wait for the semaphore to have a certain positive non-zero value.
- `cds_sync_barrier_t` -- Lets users specify a barrier that all
  threads must reach before any thread can proceed.
  
Key Features / Design Goals
---------------------------
- **Identical API on all supported platforms**. The following
  platforms are tested regularly:
  - Microsoft Windows 7
    - Visual Studio 2010
    - Visual Studio 2012
    - Visual Studio 2013
  - Linux Mint
    - LLVM/Clang 3.5
    - gcc 4.8.4
  - Apple OSX
    - Apple LLVM/Clang 6.1.0
- **No (mandatory) external dependencies**. Only standard C library
  functions are used, and even those can be overriden with custom
  implementations through `#define`s if desired.
- **Dirt-simple integration**. Just a single header file to include in
your project.
- **Public domain license terms**. 

Acknowledgements
----------------
- [Sean Barrett](http://nothings.org/): master of single-header C libraries.
- [Charles Bloom](http://cbloomrants.blogspot.com/): poster of sync primitives; ranter.
- [Allen B. Downey](http://www.allendowney.com/): author of [The Little Book of Semaphores](http://greenteapress.com/semaphores/index.html).
