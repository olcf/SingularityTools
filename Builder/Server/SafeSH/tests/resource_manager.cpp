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
        std::unique_ptr<builder::ResourceManager> resources_a;
        std::unique_ptr<builder::ResourceManager> resources_b;
        std::unique_ptr<builder::ResourceManager> resources_c;

        // Initialize resource managers
        std::thread a([&]() {
            resources_a = std::make_unique<builder::ResourceManager>();
        });
        std::thread b([&]() {
            resources_b = std::make_unique<builder::ResourceManager>();

        });
        std::thread c([&]() {
            resources_c = std::make_unique<builder::ResourceManager>();
        });
        a.join();
        b.join();
        c.join();

        // Reserve slots in parallel from each resource manager instance
        // Trying to reserve more than available slots will return false
        std::thread d([&]() {
            REQUIRE(resources_a->reserve_slot() == true);
        });
        std::thread e([&]() {
            REQUIRE(resources_b->reserve_slot() == true);
        });
        std::thread f([&]() {
            REQUIRE(resources_c->reserve_slot() == false);
        });
        d.join();
        e.join();
        f.join();
    }

    SECTION("Resources should be free'd one resource deconstruction") {
        builder::ResourceManager resources;
        REQUIRE(resources.reserve_slot() == true);
    }
}