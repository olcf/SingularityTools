#include "resource_manager.h"
#include "utilities.h"
#include "catch.hpp"
#include <thread>


TEST_CASE("ResourceManager() can be constructed") {
    TMP_DB db;

    SECTION("No exception is throw when constructed") {
        REQUIRE_NOTHROW(builder::ResourceManager());
    }

    SECTION("When constructed no slot is reserved") {
        builder::ResourceManager resources;
        REQUIRE(resources.slot_reserved() == false);
    }
}

TEST_CASE("Slots can be reserved and released") {
    TMP_DB db;

    SECTION("All the available slots can be reserved but no more") {
        std::thread a([&]() {
            builder::ResourceManager resources;
            REQUIRE(resources.reserve_slot() == true);
        });
        std::thread b([&]() {
            builder::ResourceManager resources;
            REQUIRE(resources.reserve_slot() == true);
        });
        std::thread c([&]() {
            builder::ResourceManager resources;
            REQUIRE(resources.reserve_slot() == true);
        });

        a.join();
        b.join();
        c.join();
    }
}