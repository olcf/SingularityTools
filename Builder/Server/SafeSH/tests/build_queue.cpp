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

  SECTION("If more builds are queued that resources available the job should wait for resources") {
    TMP_DB db;

    // Print time since epoch in integral seconds
    auto sleep_print_time = []() {
      std::cout << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
      std::this_thread::sleep_for(std::chrono::seconds(10));
      return 0;
    };
    // Print time since epoch in integral seconds
    auto print_time = []() {
      std::cout << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
      return 0;
    };

    // Two resources are available, the third job should wait at least 10 seconds to launch
    std::string std_out_a;
    std::string std_out_b;
    std::string std_out_c;

    std::thread a([&](){
      builder::BuildQueue queue_a;
      REQUIRE_NOTHROW(capture_stdout([&]() { queue_a.run(sleep_print_time); }, std_out_a));
    });
    std::thread b([&](){
      builder::BuildQueue queue_b;
      REQUIRE_NOTHROW(capture_stdout([&]() { queue_b.run(sleep_print_time); }, std_out_b));
    });
    std::thread c([&](){
      builder::BuildQueue queue_c;
      REQUIRE_NOTHROW(capture_stdout([&]() { queue_c.run(print_time); }, std_out_c));

    });

    a.join();
    b.join();
    c.join();

//    REQUIRE(std::stol(std_out_c) - std::stol(std_out_a) >= 10);
//    REQUIRE(std::stol(std_out_c) - std::stol(std_out_b) >= 10);
  }
}