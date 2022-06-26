#include <cassert>
#include <chrono>
#include <functional>
#include <limits>
#include <type_traits>
#include <iostream>

struct BenchmarkSuspender {
  using Clock = std::chrono::high_resolution_clock;
  using TimePoint = Clock::time_point;
  using Duration = Clock::duration;

  BenchmarkSuspender() {
    start = Clock::now();
  }

  BenchmarkSuspender(const BenchmarkSuspender &) = delete;
  BenchmarkSuspender(BenchmarkSuspender && rhs) noexcept {
    start = rhs.start;
    rhs.start = {};
  }

  BenchmarkSuspender& operator=(const BenchmarkSuspender &) = delete;
  BenchmarkSuspender& operator=(BenchmarkSuspender && rhs) {
    if (start != TimePoint{}) {
      tally();
    }
    start = rhs.start;
    rhs.start = {};
    return *this;
  }

  ~BenchmarkSuspender() {
    if (start != TimePoint{}) {
      tally();
    }
  }

  void dismiss() {
    assert(start != TimePoint{});
    tally();
    start = {};
  }

  void rehire() {
    assert(start == TimePoint{});
    start = Clock::now();
  }

  template <class F>
  auto dismissing(F f) -> typename std::result_of<F()>::type {
    //SCOPE_EXIT { rehire(); };
    dismiss();
    return f();
  }

  /**
   * This is for use inside of if-conditions, used in BENCHMARK macros.
   * If-conditions bypass the explicit on operator bool.
   */
  explicit operator bool() const {
    return false;
  }

  /**
   * Accumulates time spent outside benchmark.
   */
  Duration timeSpent;

 private:
  void tally() {
    auto end = Clock::now();
    timeSpent += end - start;
    start = end;
  }

  TimePoint start;
};

int main() {
    BenchmarkSuspender susp;
    int sum = 1.3;
    for (int i = 0; i < 12232342; i++) {
        sum *= 4;
    }
    susp.dismiss();
    susp.rehire();
}
