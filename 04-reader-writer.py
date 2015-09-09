# reader/writer problem
# this works, but writers can starve.
mutex = Semaphore(1)
writeLock = Semaphore(1)
readerCount = 0
reader:
  mutex.wait()
    readerCount += 1
    if readerCount == 1: writeLock.wait()
  mutex.signal()
  [read read read]
  mutex.wait()
    readerCount -= 1
    if readerCount == 0: writeLock.signal()
  mutex.signal()
writer:
  writeLock.wait()
    [write write write]
  writeLock.signal()

# The pattern in the reader code above can be encapsulated as a LightSwitch.
# One or more threads call ls.lock() as they enter a critical section, and
# unlock() as they leave. The lightswitch ensures that the first thread
# to enter "turns on the lights" (i.e. waits on a semaphore), and the last
# to leave "turns them off again" (i.e. signals the semaphore).
class LightSwitch:
  def __init__(self):
    self.mutex = Semaphore(1)
    self.count = 0
  def lock(self,sem):
    self.mutex.wait()
    self.count += 1
    if self.count == 1:
      sem.wait()
    self.mutex.signal()
  def unlock(self,sem):
    self.mutex.wait()
    self.count -= 1
    if self.count == 0:
      sem.signal()
    self.mutex.signal()

# reader/writer w/LightSwitch (same code as above, just simpler)
# writers are still subject to starvation
writeLock = Semaphore(1)
readSwitch = LightSwitch()
reader:
  readSwitch.lock(writeLock)
    [read read read]
  readSwitch.unlock(writeLock)
writer:
  writeLock.wait()
    [write write write]
  writeLock.signal()

# reader/writer w/un-starvable writers
# no new readesr can enter the critical section while a writer is waiting.
# PROBLEMS:
# - readers can still be starved by writers. This is dismissed as acceptable.
# - writers may require higher-priority access than readers. This may be
#   possible at the OS level, but can also be enforced in software; see
#   below.
writeLock = Semaphore(1)
readSwitch = LightSwitch()
turnstile = Semaphore(1)
reader:
  turnstile.wait()
  turnstile.signal()
  readSwitch.lock(writeLock)
    [read read read]
  readSwitch.unlock(writeLock)
writer:
  turnstile.wait()
    writerLock.wait()
      [write write write]
    writerLock.signal()
  turnstile.signal()

# reader/writer with unstarveable high-priority writers.
# If a writer is waiting, no new readers may enter the critical section.
writeLock = Semaphore(1)
readSwitch = LightSwitch()
noReaders = Semaphore(1)
noWriters = Semaphore(1)
reader:
  noWriters.wait()
  readSwitch.lock(writeLock)
    [read read read]
  readSwitch.unlock(writeLock)
writer:
  turnstile.wait()
    writerLock.wait()
      [write write write]
    writerLock.signal()
  turnstile.signal()
