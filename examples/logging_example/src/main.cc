#include <cactus_rt/rt.h>
#include <spdlog/spdlog.h>

using Fifo = cactus_rt::schedulers::Fifo;
class CyclicThread : public cactus_rt::CyclicThread<Fifo> {
 public:
  CyclicThread(std::vector<size_t> cpu_affinity) : cactus_rt::CyclicThread<Fifo>("MyThread", 1'000'000 /* 1s */, Fifo::Config{80}, cpu_affinity) {}

 protected:
  bool Loop(int64_t /* now */) noexcept final {
    return false;
  }
};

class App : public cactus_rt::App {
  CyclicThread thread_;

 public:
  App(std::vector<size_t> cpu_affinity) : thread_(cpu_affinity) {
    RegisterThread(thread_);
  }

  void Stop() {
    thread_.RequestStop();
    thread_.Join();
  }
};

int main() {
  App                    app(std::vector<size_t>{2});
  constexpr unsigned int time = 5;
  app.Start();
  sleep(time);
  app.Stop();
  return 0;
}
