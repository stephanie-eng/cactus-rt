#include "cactus_rt/app.h"

#include <malloc.h>
#include <spdlog/spdlog.h>
#include <sys/mman.h>

#include <stdexcept>

#include "cactus_rt/utils.h"

namespace cactus_rt {
void App::Start() {
  SPDLOG_DEBUG("Starting application");

  LockMemory();
  ReserveHeap();
  start_monotonic_time_ns_ = NowNs();
  start_wall_time_ns_ = WallNowNs();
}

void App::ReserveHeap() const {
  if (heap_size_ == 0) {
    // Don't reserve anything
    return;
  }

  void* buf = malloc(heap_size_);
  if (buf == nullptr) {
    SPDLOG_ERROR("cannot malloc: {}", std::strerror(errno));
    throw std::runtime_error{"cannot malloc"};
  }

  // There is no need the poke each page in the buffer to ensure that the page
  // is actually allocated, because mlockall effectively turns off demand
  // paging. See mlockall(2) and "demand paging" on Wikipedia. Also see:
  // https://github.com/ros2-realtime-demo/pendulum/issues/90#issuecomment-1105844726

  free(buf);
}

void App::LockMemory() const {
  // See https://lwn.net/Articles/837019/

  // From the man page:
  //
  // Locks all pages mapped into the address space, including code, data, stack
  // segments, shared libraries, user space kernel data, shared memory, and
  // memory mapped files.
  //
  // All mapped pages are guaranteed to be resident in RAM when the call
  // returns successfully; the pages are guaranteed in RAM until later
  // unlocked.
  //
  // MCL_CURRENT: Lock all pages which are currently mapped into the address
  //              space of the process.
  //
  // MCL_FUTURE: Lock all pages which will become mapped into the address space
  //             of the process in the future. These could be, for instance,
  //             new pages required by a growing heap and stack as well as new
  //             memory mapped files and shared memory regions.
  int ret = mlockall(MCL_CURRENT | MCL_FUTURE);
  if (ret != 0) {
    SPDLOG_ERROR("mlockall failed: {}", std::strerror(errno));
    throw std::runtime_error{"mlockall failed"};
  }

  // Do not free any RAM to the OS if the continguous free memory at the top of
  // the heap grows too large. If RAM is freed, a syscall (sbrk) will be called
  // which can have unbounded execution time.
  ret = mallopt(M_TRIM_THRESHOLD, -1);
  if (ret == 0) {
    // on error, errno is not set by mallopt
    throw std::runtime_error{"mallopt M_TRIM_THRESHOLD failed"};
  }

  // Do not allow mmap.
  // TODO: give example why this is bad.
  ret = mallopt(M_MMAP_MAX, 0);
  if (ret == 0) {
    throw std::runtime_error{"mallopt M_TRIM_THRESHOLD failed"};
  }
}
}  // namespace cactus_rt
