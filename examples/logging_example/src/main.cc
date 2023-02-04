#include <cactus_rt/rt.h>
#include <spdlog/spdlog.h>

using Fifo = cactus_rt::schedulers::Fifo;
using Other = cactus_rt::schedulers::Other;

class CyclicFifoThread : public cactus_rt::CyclicThread<Fifo> {
 public:
  CyclicFifoThread(std::vector<size_t> cpu_affinity) : cactus_rt::CyclicThread<Fifo>("MyThread", 1'000'000 /* 1s */, Fifo::Config{80}, cpu_affinity) {}

 protected:
  bool Loop(int64_t /* now */) noexcept final {
    Log("hello world");
    return false;
  }
};

class App : public cactus_rt::App {
  CyclicFifoThread rt_thread_;

 public:
  App(std::vector<size_t> cpu_affinity) : rt_thread_(cpu_affinity) {
    RegisterThread(rt_thread_);
  }

  void Stop() {
    rt_thread_.RequestStop();
    rt_thread_.Join();
  }
};

int main(int /* argc */, char** /* argv[] */) {
  spdlog::set_level(spdlog::level::debug);
  App                    app(std::vector<size_t>{2});
  constexpr unsigned int time = 5;
  app.Start();
  sleep(time);

  app.Stop();
  return 0;
}
