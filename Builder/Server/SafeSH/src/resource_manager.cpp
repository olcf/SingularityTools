#include "build_queue.h"
#include <iostream>
#include <string>
#include <system_error>
#include "sql_db.h"
#include <thread>

namespace builder {
#ifdef DEBUG
    static constexpr auto resource_database = "./ResourceManager.db";
#else
    static constexpr auto resource_database = "/home/builder/ResourceManager.db";
#endif

    ResourceManager::ResourceManager() : db{resource_database},
                                         slot_id("") {}

    ResourceManager::~ResourceManager() {
        this->release_slot(NO_THROW);
    }

    bool ResourceManager::slot_reserved() {
        return !this->slot_id.empty();
    }

    // Release slot slot
    void ResourceManager::release_slot(bool should_throw) {
        if (!this->slot_reserved())
            return;
        this->set_status(SlotStatus::free, should_throw);
        this->slot_id = "";
    }

    // TODO: Clean this up so we don't leak transactions or such
    // Reserve a build slot if one is available, return true if reserved else return false
    static int slot_available_callback(void *available_slot_id, int count, char **values, char **names) {
        *static_cast<std::string *>(available_slot_id) = values[0];
        return 0;
    }

    bool ResourceManager::reserve_slot() {

        if (this->slot_reserved()) {
            std::cerr << "Slot already reserved!\n";
            return true;
        }
        // Begin immediate transaction, if not immediate multiple processes may read
        // and attempt to update an available row
        db.exec("BEGIN IMMEDIATE TRANSACTION", nullptr, nullptr);

        // See if a slot is available
        std::string available_slot_id;
        std::string select_command =
                std::string() + "SELECT id FROM slot WHERE status = \"" + static_cast<char>(SlotStatus::free) +
                "\" LIMIT 1;";
        db.exec(select_command, slot_available_callback, &available_slot_id);

        // If no slot is free return
        if (available_slot_id.empty()) {
            db.exec("END TRANSACTION", nullptr, nullptr);
            return false;
        }

        // If a slot is available reserve it
        this->slot_id = available_slot_id;
        this->set_status(SlotStatus::reserved);

        // End transaction
        db.exec("END TRANSACTION", nullptr, nullptr);

        this->slot_id = available_slot_id;
        return true;
    }

    void ResourceManager::set_status(SlotStatus status, bool should_throw) {
        std::string status_command = std::string() + "UPDATE slot SET status = \"" + static_cast<char>(status) +
                                     "\" WHERE id = \"" + this->slot_id + "\";";
        db.exec(status_command, nullptr, nullptr, should_throw);
    }

}
