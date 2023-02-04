#include <boost/lockfree/queue.hpp>

#include "../cyclic_thread.h"
#include "../schedulers/other.h"

using boost::lockfree::queue;

namespace cactus_rt::background_threads {
class Logger : public cactus_rt::CyclicThread<schedulers::Other> {
 public:
  Logger() : CyclicThread<schedulers::Other>("Logger", 1'000'000 /* 1s */, schedulers::Other::Config()) {}

 protected:
  bool Loop(int64_t /* now */) noexcept final;
};
}  // namespace cactus_rt::background_threads
