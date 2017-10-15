#include "build_queue.h"
#include <iostream>
#include <string>
#include <system_error>
#include "sql_db.h"

namespace builder {
  #ifdef DEBUG
    static constexpr auto resource_database = "./ResourceManager.db";
  #else
    static constexpr auto resource_database = "/home/builder/ResourceManager.db";
  #endif

  ResourceManager::ResourceManager() : db{resource_database}
  {}

  ResourceManager::~ResourceManager() {
      this->release_slot(NO_THROW);
  }

  bool ResourceManager::slot_reserved() {
    return !this->slot_id.empty();
  }

  // Release slot slot
  void ResourceManager::release_slot(bool should_throw) {
    if(!this->slot_reserved())
      return;

    this->set_status(SlotStatus::free, should_throw);
    this->slot_id = "";
  }

  // Reserve a build slot if one is available, return true if reserved else return false
  bool ResourceManager::reserve_slot() {
    if(this->slot_reserved()) {
      std::cerr<<"Slot already reserved!\n";
      return true;
    }

    // Reserve a free slot if one exists
    std::string update_command = std::string() + "UPDATE slot SET status = " +
                                 static_cast<char>(SlotStatus::reserved) +
                                 " WHERE status = " + static_cast<char>(SlotStatus::free) + " LIMIT 1;";
    db.exec(update_command, NULL, NULL);

    // Check if we were able to reserve the slot and update as required
    int reserved_slot = db.changes();
    if(reserved_slot) {
      this->slot_id = std::to_string(db.last_insert_rowid());
    }

    return reserved_slot;
  }

  void ResourceManager::set_status(SlotStatus status, bool should_throw) {
    std::string status_command = std::string() + "UPDATE slot SET status = " + static_cast<char>(status) +
                                 " WHERE id = " + this->slot_id + ";";
    db.exec(status_command, NULL, NULL, NO_THROW);
  }

}
