#include "catch.hpp"
#include <cstdlib>
#include "ssh_sanitizer.h"
#include <boost/filesystem.hpp>
#include "utilities.h"
#include "build_queue.h"
#include "chrono"
#include "thread"

using Catch::Contains;
using Catch::Equals;

TEST_CASE("run() works correctly") {

    SECTION("run should return 0 on success") {
        builder::BuildQueue queue;
        auto func = [](std::string slot_id) -> int {
            return 0;
        };
        REQUIRE(queue.run(func) == 0);
    }

    SECTION("run() should run the provided function") {
        builder::BuildQueue queue;
        auto func = [](std::string slot_id) -> std::string {
            return std::string("Running!");
        };

        REQUIRE_THAT(queue.run(func), Equals("Running!"));
    }

    SECTION("BuildQueues should run statements in the order they were queued") {
        // Print time since epoch in integral milliseconds
        auto start_time = [](std::string slot_id) {
            return std::chrono::system_clock::now();
        };

        std::chrono::time_point<std::chrono::system_clock> a_time, b_time;

        std::thread a([&]() {
            builder::BuildQueue queue_a;
            REQUIRE_NOTHROW(a_time = queue_a.run(start_time));
        });
        std::this_thread::sleep_for(std::chrono::seconds(10));
        std::thread b([&]() {
            builder::BuildQueue queue_b;
            REQUIRE_NOTHROW(b_time = queue_b.run(start_time));
        });
        a.join();
        b.join();

        REQUIRE(a_time < b_time);
    }

    // This test isn't guaranteed to work as there i no guarantee
    // as to the order in which the threads will be launched.
    // With that said it should likely work
    SECTION("jobs should run in the queue in FIFO order") {
        // Return the current time and then sleep for 10 seconds
        auto start_time_then_sleep = [](std::string slot_id) {
            auto time = std::chrono::system_clock::now();
            std::this_thread::sleep_for(std::chrono::seconds(10));
            return time;
        };
        //  Return the current time
        auto start_time = [](std::string slot_id) {
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

        // a and b should run then ~10 seconds later c should  run
        REQUIRE(std::chrono::seconds(9) <= c_time - a_time);
        REQUIRE(std::chrono::seconds(9) <= c_time - b_time);
        REQUIRE(std::chrono::seconds(2) >= b_time - a_time);
    }
}