#include "cactus_rt/thread.h"

#include <spdlog/spdlog.h>

#include <cstring>
#include <stdexcept>

namespace cactus_rt {
void* Thread::RunThread(void* data) {
  auto* thread = static_cast<Thread*>(data);

  thread->BeforeRun();

  bool is_rt = thread->policy_ == SCHED_FIFO || thread->policy_ == SCHED_DEADLINE || thread->policy_ == SCHED_RR;
  if (is_rt) {
    SPDLOG_DEBUG("starting the RT thread {}", thread->Name());
    int ret = getrusage(RUSAGE_THREAD, &thread->page_faults_at_start_);
    if (ret != 0) {
      SPDLOG_ERROR("unable to getrusage: {}", std::strerror(errno));
      throw std::runtime_error{"failed to getrusage"};
    }
  }

  thread->Run();

  if (is_rt) {
    struct rusage page_fault_at_end;

    int ret = getrusage(RUSAGE_THREAD, &page_fault_at_end);
    if (ret != 0) {
      SPDLOG_WARN("unable to getrusage at the end (skipping sanity check): {}", std::strerror(errno));
    } else {
      auto minor_faults = page_fault_at_end.ru_minflt - thread->page_faults_at_start_.ru_minflt;
      auto major_faults = page_fault_at_end.ru_majflt - thread->page_faults_at_start_.ru_majflt;

      if (minor_faults > 0 || major_faults > 0) {
        SPDLOG_ERROR("page faults detected for thread {}: {} minor, {} major", thread->Name(), minor_faults, major_faults);
      }
    }

    SPDLOG_DEBUG("RT thread {} finished", thread->Name());
  }

  thread->AfterRun();

  return nullptr;
}

void Thread::Start(int64_t start_monotonic_time_ns, int64_t start_wall_time_ns) {
  start_monotonic_time_ns_ = start_monotonic_time_ns;
  start_wall_time_ns_ = start_wall_time_ns;

  pthread_attr_t attr;

  // Initialize the pthread attribute
  int ret = pthread_attr_init(&attr);
  if (ret != 0) {
    throw std::runtime_error(std::string("error in pthread_attr_init: ") + std::strerror(ret));
  }

  // Set a stack size
  //
  // Note the stack is prefaulted if mlockall(MCL_FUTURE | MCL_CURRENT) is
  // called, which under this app framework should be.
  //
  // Not even sure if there is an optimizer-safe way to prefault the stack
  // anyway, as the compiler optimizer may realize that buffer is never used
  // and thus will simply optimize it out.
  ret = pthread_attr_setstacksize(&attr, stack_size_);
  if (ret != 0) {
    throw std::runtime_error(std::string("error in pthread_attr_setstacksize: ") + std::strerror(ret));
  }

  // Set the scheduler policy
  ret = pthread_attr_setschedpolicy(&attr, policy_);
  if (ret != 0) {
    throw std::runtime_error(std::string("error in pthread_attr_setschedpolicy: ") + std::strerror(ret));
  }

  // Set the scheduler priority
  struct sched_param param;
  param.sched_priority = priority_;
  ret = pthread_attr_setschedparam(&attr, &param);
  if (ret != 0) {
    throw std::runtime_error(std::string("error in pthread_attr_setschedparam: ") + std::strerror(ret));
  }

  // Make sure threads created using the thread_attr_ takes the value from the attribute instead of inherit from the parent thread.
  ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  if (ret != 0) {
    throw std::runtime_error(std::string("error in pthread_attr_setinheritsched: ") + std::strerror(ret));
  }

  // Setting CPU mask
  if (!cpu_affinity_.empty()) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (auto cpu : cpu_affinity_) {
      CPU_SET(cpu, &cpuset);
    }

    ret = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
    if (ret != 0) {
      throw std::runtime_error(std::string("error in pthread_attr_setaffinity_np: ") + std::strerror(ret));
    }
  }

  ret = pthread_create(&thread_, &attr, &Thread::RunThread, this);
  if (ret != 0) {
    throw std::runtime_error(std::string("error in pthread_create: ") + std::strerror(ret));
  }
}

int Thread::Join() const {
  return pthread_join(thread_, nullptr);
}

}  // namespace cactus_rt
