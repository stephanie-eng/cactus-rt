#include <cactus_rt/background_threads/logger.h>

namespace cactus_rt::background_threads {
bool Logger::Loop(int64_t /* now */) noexcept {
  bool  no_data = true;
  char* msg;
  if (log_queue_->pop(msg)) {
    SPDLOG_INFO(msg);
  }
}
}  // namespace cactus_rt::background_threads
