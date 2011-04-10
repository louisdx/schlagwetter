#ifndef H_SYNCQUEUE
#define H_SYNCQUEUE


#include <deque>
#include <mutex>

/*  Class SyncQueue: Thread-safe queueing. Modelled on std::deque,
 *  but always adds at the back and removes from the front.
 */

class SyncQueue
{
public:
  SyncQueue() : m_queue(), m_utex() { }

  typedef std::recursive_mutex Mutex;

  // Lockable concept.
  inline void lock() { m_utex.lock(); }
  inline void unlock() { m_utex.unlock(); }

  inline void push(unsigned char x)
  {
    std::lock_guard<Mutex> lock(m_utex);
    m_queue.push_back(x);
  }

  inline void pushMaybeAndClear(std::deque<unsigned char> & queue)
  {
    if (m_utex.try_lock())
    {
      m_queue.insert(m_queue.end(), queue.begin(), queue.end());
      m_utex.unlock();
      queue.clear();
    }
    else
    {
      std::cerr << "Warning: SyncQueue was locked; consider reworking the thread model." << std::endl;
    }
  }

  // This one is only needed to rewind an aborted read.
  template <typename T> inline void pushFrontUnsafe(T beg, T end)
  {
    m_queue.insert(m_queue.begin(), beg, end);
  }

  inline unsigned char pop()
  {
#if DEBUG
    if (m_queue.empty()) { throw "Panic in SyncQueue::pop()!"; }
#endif

    std::lock_guard<Mutex> lock(m_utex);
    unsigned char x = m_queue.front();
    m_queue.pop_front();
    return x;
  }

  inline unsigned char pop_unsafe()
  {
#if DEBUG
    if (m_queue.empty()) { throw "Panic in SyncQueue::pop()!"; }
#endif

    unsigned char x = m_queue.front();
    m_queue.pop_front();
    return x;
  }

  inline unsigned char front()
  { 
#if DEBUG
    if (m_queue.empty()) { throw "Panic in SyncQueue::front()!"; }
#endif

    std::lock_guard<Mutex> lock(m_utex);
    return m_queue.front();
  }

  inline void clear()
  { 
    std::lock_guard<Mutex> lock(m_utex);
    m_queue.clear();
  }

  inline size_t size() const { return m_queue.size(); }
  inline bool  empty() const { return m_queue.empty(); }

  // Hush. This isn't supposed to be here.
  const std::deque<unsigned char> & q() const { return m_queue; }

private:
  std::deque<unsigned char> m_queue;
  Mutex m_utex;
};


#endif
