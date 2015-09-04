# My original version: works, not reusable, no turnstile.
# (but it's actually closer to the spirit of the final version; see below)
global count = 0
global mutex = Semaphore(1)
global sem = Semaphore(0)
const global threadCount = 5
local isLast = false
mutex.wait()
  count += 1
  isLast = (count == threadCount)
mutex.signal()
if isLast:
  sem.signal() * 5
sem.wait()
[critical section]    

# Here's a version with a turnstile
# problem: semaphore value is uncertain when the operation is complete.
# problem: if the barrier is in a loop, threads can finish and re-enter
#          before all threads have left, wreaking havoc.
global count = 0
global mutex = Semaphore(1)
global turnstile = Semaphore(0)
const global threadCount = 5
mutex.wait()
  count += 1
mutex.signal()
if count == threadCount:
   turnstile.signal() # may happen >1 time
turnstile.wait()
# one thread at a time comes through here, but only after all threads
# pass the rendezvous
turnstile.signal()
[critical section]

# Reusable barrier (turnstile edition)
global count  = 0
global mutex  = Semaphore(1)
global turnstileIn  = Semaphore(0) # initially locked
global turnstileOut = Semaphore(1) # initially unlocked
const global threadCount = 5
# precondition: turnstileIn = 0 (locked), turnstileOut = 1 (unlocked)
mutex.wait()
  count += 1
  if count == threadCount:
    turnstileOut.wait()  # lock the exit (unlocked per precond; no deadlock)
    turnstileIn.signal() # unlock the entrance
mutex.signal()
turnstileIn.wait()
# one thread at a time comes through here, but only after all threads
# pass the rendezvous
turnstileIn.signal()
[critical section]
mutex.wait()
  count -= 1
  if count == 0:
    turnstileIn.wait()  # lock the entrance (unlocked above; no deadlock)
    turnstileOut.signal() # unlock the exit
mutex.signal()
turnstileOut.wait()
# one thread at a time here
turnstileOut.signal()
# postcondition: turnstileIn = 0 (locked), turnstileOut = 1 (open)

# Simplified/Optimized reusable barrier (preloaded turnstiles)
# To minimize context switches, the thread that unlocks the turnstile
# can just signal N times up front instead of having each thread signal
# as it passes through.
# So, are these even turnstiles anymore? Doesn't seem like it.
global count  = 0
global mutex  = Semaphore(1)
global semIn  = Semaphore(0) # initially locked
global semOut = Semaphore(0) # initially locked
const global threadCount = 5
# precondition: semIn = 0 (locked), semOut = 0 (locked)
mutex.wait()
  count += 1
  if count == threadCount:
    semIn.signalN(threadCount) # unlock the entrance for everyone
mutex.signal()
semIn.wait() # last thread through here locks semIn behind it
[critical section]
mutex.wait()
  count -= 1
  if count == 0:
    semOut.signalN(threadCount) # unlock the exit for everyone
mutex.signal()
semOut.wait() # last thread through here locks semOut behind it
# postcondition: semIn = 0 (locked), semOut = 0 (locked)

# as a reusable object:
# - if you just all threads to wait at the barrier until they've all arrived:
#   barrier.waitAt()
# - if you have code to run while all threads are inside the barrier:
#   barrier.enter()
#   [critical section]
#   barrier.exit()
class Barrier(threadCount):
  self.insideCount = 0
  self.threadCount = threadCount
  self.mutex  = Semaphore(1)
  self.semIn  = Semaphore(0)
  self.semOut = Semaphore(0)
def enter():
  self.mutex.wait()
  self.insideCount += 1
  if self.insideCount == self.threadCount:
    self.semIn.signalN(self.threadCount)
  self.mutex.signal()
  self.semIn.wait()
def exit():
  self.mutex.wait()
  self.insideCount -= 1
  if self.insideCount == 0:
    self.semOut.signalN(self.threadCount)
  self.mutex.signal()
  self.semOut.wait()
def waitAt():
  self.enter()
  self.exit()
