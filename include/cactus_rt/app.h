#ifndef CACTUS_RT_APP2_H_
#define CACTUS_RT_APP2_H_

#include <boost/lockfree/queue.hpp>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>

#include "background_threads/logger.h"
#include "thread.h"

namespace cactus_rt {
class App {
  /**
   * Size of the heap to reserve in bytes.
   */
  size_t heap_size_;

  boost::lockfree::queue<const char*, boost::lockfree::capacity<1024>> log_queue_;

  // Non-owning references to threads just to help with starting and joining the thrad.
  std::vector<BaseThread*>   threads_;
  background_threads::Logger logger_;

 public:
  /**
   * Creates an instance of the RT app. The app should always be created before
   * the threads as there are some global setup that can take place.
   *
   * @param heap_size The heap size to reserve in bytes. Defaults to 512MB.
   */
  explicit App(size_t heap_size = 512 * 1024 * 1024) : heap_size_(heap_size) {
    logger_.log_queue_ = &log_queue_;
  }
  virtual ~App() = default;

  // Copy constructors
  App(const App&) = delete;
  App& operator=(const App&) = delete;

  // Move constructors
  App(App&&) noexcept = default;
  App& operator=(App&&) noexcept = default;

  /**
   * @brief Registers a thread to be automatically started by the app. The start
   * order of the threads are in the order of registration.
   *
   * @param thread A reference to the thread. Note that this function call does
   * not assume ownership of the thread. Ensure the lifetime of the thread is at
   * least as long as the lifetime of the App.
   */
  void RegisterThread(BaseThread& thread);

  /**
   * Starts the app by locking the memory and reserving the memory. Also start
   * all the threads in registration order.
   */
  virtual void Start();

  /**
   * Joins all the threads in registration order.
   *
   * Override this if you want a different order of operation, or if you want to
   * request stop on a thread after another one is done.
   */
  virtual void Join();

 protected:
  /**
   * Locks memory via mlockall and configure malloc to not give up memory nor
   * use mmap.
   */
  void LockMemory() const;

  /**
   * Reserve the heap based on the heap_size_.
   */
  void ReserveHeap() const;
};
}  // namespace cactus_rt

#endif
