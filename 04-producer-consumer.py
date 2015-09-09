# My first stab at producer/consumer
mutex = Semaphore(1)
items = Semaphore(0)
producer:
  event = waitForEvent()
  mutex.wait()
    buffer.add(event)
  mutex.signal() # unlock mutex before signaling consumers, for efficiency
  items.signal() # (otherwise they'll wake up & block again immediately)
consumer:
  items.wait()
  mutex.wait()
    event = buffer.get()
  mutex.signal()
  event.process()

# Producer/consumer with a finite buffer
mutex = Semaphore(1)
items = Semaphore(0)
freeslots = Semaphore(bufferSize)
producer:
  event = waitForEvent()
  freeslots.wait()
  mutex.wait()
    buffer.add(event)
  mutex.signal() # unlock mutex before signaling consumers, for efficiency
  items.signal() # (otherwise they'll wake up & block again immediately)
consumer:
  items.wait()
  mutex.wait()
    event = buffer.get()
  mutex.signal()
  freeslots.signal()
  event.process()
