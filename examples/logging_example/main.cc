#include <cactus_rt/rt.h>

#include <iostream>

using cactus_rt::App;
using cactus_rt::CyclicThread;

/**
 * This is a no-op thread that does nothing at 1 kHz.
 *
 */
class ExampleRTThread : public CyclicThread {
  int64_t loop_counter_ = 0;

 public:
  ExampleRTThread(cactus_rt::CyclicThreadConfig config) : CyclicThread(config
                                                          ) {}

  int64_t GetLoopCounter() const {
    return loop_counter_;
  }

 protected:
  bool Loop(int64_t /*now*/) noexcept final {
    loop_counter_++;
    if (loop_counter_ % 1000 == 0) {
      LOG_INFO(Logger(), "Loop {}", loop_counter_);
    }
    return false;
  }
};

int main() {
  cactus_rt::FifoThreadConfig fifo_config;
  fifo_config.priority = 80;

  cactus_rt::CyclicThreadConfig config;
  config.name = "ExampleRTThread";
  config.period_ns = 1'000'000;
  config.cpu_affinity = std::vector<size_t>{2};
  config.scheduler_config = fifo_config;

  auto thread = std::make_shared<ExampleRTThread>(config);

  // Create a cactus_rt app configuration
  cactus_rt::AppConfig app_config;

  // Create a Quill logging config to configure logging
  quill::Config logging_config;

  // Disable strict timestamp order - this will be faster, but logs may appear out of order
  logging_config.backend_thread_strict_log_timestamp_order = false;

  // Set the background logging thread CPU affinity
  logging_config.backend_thread_cpu_affinity = 1;  // Different CPU than the CyclicThread CPU!

  app_config.logger_config = logging_config;
  App app(app_config);

  app.RegisterThread(thread);
  constexpr unsigned int time = 5;

  std::cout << "Testing RT loop for " << time << " seconds.\n";

  app.Start();
  sleep(time);
  app.RequestStop();
  app.Join();

  std::cout << "Number of loops executed: " << thread->GetLoopCounter() << "\n";
  return 0;
}