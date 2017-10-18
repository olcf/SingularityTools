#include "catch.hpp"
#include <stdlib.h>
#include "ssh_sanitizer.h"
#include "sql_db.h"
#include <boost/filesystem.hpp>
#include "utilities.h"
#include "build_queue.h"
#include "chrono"
#include "thread"

using Catch::Contains;
using Catch::Equals;

TEST_CASE("BuildQueue() can be constructed") {
  SECTION("No exception is throw when constructed") {
    TMP_DB db;
    REQUIRE_NOTHROW(builder::BuildQueue());
  }
}

TEST_CASE("run() works correctly") {
  SECTION("run should return 0 on success") {
    TMP_DB db;
    builder::BuildQueue queue;
    auto func = []() {
      return 0;
    };
    int rc = queue.run(func);
    REQUIRE(rc == 0);
  }

  SECTION("run() should run the provided function") {
    TMP_DB db;
    builder::BuildQueue queue;
    auto print_func = []() {
      std::cout << "Running!";
      return 0;
    };

    std::string std_out;
    REQUIRE_NOTHROW(capture_stdout([&]() { queue.run(print_func); }, std_out));
    REQUIRE_THAT(std_out, Equals("Running!"));
  }

  SECTION("BuildQueues should run statements in the order they were queued") {
    TMP_DB db;
    builder::BuildQueue queue_a;
    sleep(5);
    builder::BuildQueue queue_b;

    // Print time since epoc in integral milliseconds
    auto print_time = []() {
      std::cout << std::chrono::system_clock::now().time_since_epoch().count();
      return 0;
    };

    std::string std_out_a;
    REQUIRE_NOTHROW(capture_stdout([&]() { queue_a.run(print_time); }, std_out_a));
    std::string std_out_b;
    REQUIRE_NOTHROW(capture_stdout([&]() { queue_b.run(print_time); }, std_out_b));
    REQUIRE(std::stol(std_out_a) < std::stol(std_out_b));
  }

  // This test isn't guaranteed to work as there i no guarantee
    // as to the order in which the threads will be launched.
    // With that said it should likely work
  SECTION("jobs should run in the queue in FIFO order") {
    for(int i=0; i<10; i++) {
      TMP_DB db;

      // Return the current time and then sleep for 10 seconds
      auto start_time_then_sleep = []() {
        auto time = std::chrono::system_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return time;
      };
      //  Return the current time
      auto start_time = []() {
        return std::chrono::system_clock::now();
      };

      // Two resources are available, the third job should wait at least 10 seconds to launch
      std::chrono::time_point<std::chrono::system_clock> a_time, b_time, c_time;

      std::thread a([&]() {
        builder::BuildQueue queue_a;
        REQUIRE_NOTHROW(a_time = queue_a.run(start_time_then_sleep));
      });
      std::thread b([&]() {
        builder::BuildQueue queue_b;
        REQUIRE_NOTHROW(b_time = queue_b.run(start_time_then_sleep));
      });
        std::this_thread::sleep_for(std::chrono::seconds(1));
      std::thread c([&]() {
        builder::BuildQueue queue_c;
        REQUIRE_NOTHROW(c_time = queue_c.run(start_time));
      });

      a.join();
      b.join();
      c.join();

      REQUIRE(std::chrono::seconds(9) <= c_time - a_time);
      REQUIRE(std::chrono::seconds(9) <= c_time - b_time);
      REQUIRE(std::chrono::seconds(2) >= b_time - a_time);
    }
  }
}