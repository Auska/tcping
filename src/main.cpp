#include "include/args.h"
#include "include/common.h"
#include "include/config.h"
#include "include/statistics.h"
#include "include/tcping.h"
#include "include/version.h"

#include <functional>
#include <future>
#include <mutex>
#include <queue>

std::atomic<bool> keep_running(true);

#ifdef _WIN32
class WinSockInitializer {
public:
  WinSockInitializer() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
      std::cerr << "WSAStartup failed" << std::endl;
      exit(1);
    }
  }
  ~WinSockInitializer() { WSACleanup(); }
};
static WinSockInitializer winSockInit;
#endif

void signalHandler(int /* signal */) {
  keep_running = false;
}

// Thread pool for parallel connections
class ThreadPool {
public:
  explicit ThreadPool(size_t threads) : stop_(false) {
    for (size_t i = 0; i < threads; ++i) {
      workers_.emplace_back([this] {
        while (true) {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(this->queue_mutex_);
            this->condition_.wait(lock, [this] {
              return this->stop_.load() || !this->tasks_.empty();
            });
            if (this->stop_.load() && this->tasks_.empty()) {
              return;
            }
            task = std::move(this->tasks_.front());
            this->tasks_.pop();
          }
          task();
        }
      });
    }
  }

  template <class F>
  auto enqueue(F&& f) -> std::future<std::invoke_result_t<F>> {
    using return_type = std::invoke_result_t<F>;
    auto task =
        std::make_shared<std::packaged_task<return_type()>>(std::forward<F>(f));
    std::future<return_type> res = task->get_future();
    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      if (stop_.load()) {
        throw std::runtime_error("enqueue on stopped ThreadPool");
      }
      tasks_.emplace([task]() { (*task)(); });
    }
    condition_.notify_one();
    return res;
  }

  ~ThreadPool() {
    stop_.store(true);
    condition_.notify_all();
    for (std::thread& worker : workers_) {
      worker.join();
    }
  }

private:
  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::mutex queue_mutex_;
  std::condition_variable condition_;
  std::atomic<bool> stop_;
};

int main(int argc, char* argv[]) {
  Config config;

  if (!ArgumentParser::parseArguments(argc, argv, config)) {
    return 1;
  }

  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  printStartupInfo(config);

  PortStatistics portStats;
  std::mutex stats_mutex;
  std::mutex output_mutex;

  ThreadPool pool(config.concurrency);

  std::vector<std::future<void>> futures;

  int attempt_count = 0;
  while (keep_running) {
    if (config.count > 0 && attempt_count >= config.count) {
      break;
    }

    // Resolve DNS once per round
    std::string resolved_ip = resolveHost(config.host, config.ipv6);

    // Check all ports in parallel
    for (int port : config.ports) {
      if (!keep_running) {
        break;
      }

      // Enqueue connection task
      int port_copy = port;
      std::string resolved_copy = resolved_ip;
      futures.push_back(pool.enqueue([&config, port_copy, resolved_copy,
                                      &portStats, &stats_mutex,
                                      &output_mutex]() {
        Tcping tcping(resolved_copy, port_copy, config.ipv6);

        double connection_time = -1.0;
        std::string error_msg;
        ConnectionState connection_state = ConnectionState::Unknown;
        bool success = tcping.checkConnection(config.timeout, &connection_time,
                                              &error_msg, &connection_state);

        {
          std::lock_guard<std::mutex> lock(stats_mutex);
          portStats.recordAttempt(port_copy, success, connection_time,
                                  connection_state);
        }

        std::string timestamp = getCurrentTimestamp();
        std::string resolved_host = tcping.getResolvedHost();
        {
          std::lock_guard<std::mutex> lock(output_mutex);
          printConnectionResult(timestamp, config.host, port_copy, success,
                                connection_time, error_msg, resolved_host,
                                config.verbose);
        }
      }));
    }

    // Wait for all ports in this round to complete
    for (auto& f : futures) {
      f.wait();
    }
    futures.clear();

    attempt_count++;

    if (keep_running && (config.count == 0 || attempt_count < config.count)) {
      std::this_thread::sleep_for(std::chrono::seconds(config.interval));
    }
  }

  std::cout << "\nMonitoring stopped." << std::endl;
  std::cout.flush();

  if (config.show_statistics) {
    portStats.printSummary();
  }

  return 0;
}
