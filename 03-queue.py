# Using semaphores to implement queues
# Leaders and followers arrive independently, and proceed into dance()
# in pairs.
# PROBLEM (potentially): the queue is not a FIFO; thread waking order is
# random.
# PROBLEM (potentially): one group could proceed much faster than the other;
# several leaders can finish dance()ing before a single follower dance()s.
queueL   = Semaphore(0)
queueF = Semaphore(0)
leader:
  queueL.signal()
  queueF.wait()
  dance()
follower:
  queueF.signal()
  queueL.wait()
  dance()

# my exclusive version -- "Dance With The One That Brought You" edition
# Seems much simpler than the reference implementation. TODO: write a 
# unit test and see if threadsanitizer can tell me if there's a race
# I'm not seeing.
queueL = Semaphore(0)
queueF = Semaphore(0)
mutexL = Semaphore(1)
mutexF = Semaphore(1)
leader:
  mutexL.wait()
    queueL.signal()
    queueF.wait()
    dance()
  mutexL.signal()

follower:
  mutexF.wait()
    queueF.signal()
    queueL.wait()
    dance()
  mutexF.signal()

# reference exclusive queue implementation:
leaderCount = 0
followerCount = 0
mutex = Semaphore(1) # protects leader/follower counts
queueL = Semaphore(0)
queueF = Semaphore(0)
rendezvous = Semaphore(0)
leader:
  mutex.wait()
    if followerCount > 0:
      followerCount -= 1
      queueF.signal()
    else:
      leaderCount += 1
      mutex.signal()
      queueL.wait()
  dance()
  rendezvous.wait()
  mutex.signal() # matches wait() by corresponding follower
follower:
  mutex.wait()
    if leaderCount > 0:
      leaderCount -= 1
      queueL.signal()
      # in this case, the signal()ed leader will signal the mutex for us
    else:
      followerCount += 1
      mutex.signal()
      queueF.wait()
  dance()
  rendezvous.signal()

# my FIFO queue
local mySem = Semaphore(0)
Fifo:
  mutex = Semaphore(1) # protects access to the queue
  queue = Queue() # plain old not-thread-safe shared queue
  wait():
    mutex.wait()
      queue.add(mySem)
    mutex.signal()
    mySem.wait()
  signal():
    if queue.empty()
      return # TODO: Is this sufficient for empty queues? Probably.
    mutex.wait()
      local nextSem = queue.remove()
    mutex.signal()
    nextSem.signal()
