#include "build_queue.h"
#include <iostream>
#include <string>
#include <system_error>
#include "sql_db.h"

// Handle allowing a maximum number of concurrent builds. 
#define MAX_VM_COUNT 4

namespace builder {
  #ifdef DEBUG
    static constexpr auto gDatabase = "./ResourceManager.db";
  #else
    static constexpr auto gDatabase = "/home/builder/ResourceManager.db";
  #endif 

  ResourceManager::ResourceManager() : db{gDatabase},
                                       build_slot_reserved{false}
  {}

  ResourceManager::~ResourceManager() {
    // Ensure that the build slot is released if not explicitly done
    if(build_slot_reserved) {
      this->release_build_slot();
    }
  }

  // If active VM count < MAX_VM_COUNT incriment VM count and return true
  // Reserve a valid loop device id if possible, or -1 if no device is available
  static int active_count_callback(void *active_vm_count, int count, char** values, char** names) {
    *static_cast<int*>(active_vm_count) = std::stoi(std::string(values[0]));
    return 0;
  }

  // Decrement the number of active build
  void ResourceManager::release_build_slot(bool should_throw) {
    std::string SQL_insert("UPDATE active_builds SET count = count - 1 WHERE id = 1;");
    db.exec(SQL_insert, NULL, NULL, should_throw);
  }

  // Reserve a build slot if one is available, return true if reserved else return false
  bool ResourceManager::reserve_build_slot() {
    if(this->build_slot_reserved) {
      throw std::system_error(EBUSY, std::system_category(), "Build slot already reserved!");
    }

    // Begin transaction
    db.exec("BEGIN TRANSACTION", NULL, NULL);

    // Read the number of active VMs
    int active_vm_count;
    std::string SQL_fetch("SELECT count FROM active_builds LIMIT 1;");
    db.exec(SQL_fetch, active_count_callback, &active_vm_count);

    // If there is space reserve a slot
    bool reserved_slot = false;
    if(active_vm_count < MAX_VM_COUNT) {
      active_vm_count++;
      std::string SQL_insert("UPDATE active_builds SET count = count + 1 WHERE id = 1;");
      db.exec(SQL_insert.c_str(), NULL, NULL);
      reserved_slot = true;
    }
    // End transaction
    db.exec("END TRANSACTION", NULL, NULL);

    this->build_slot_reserved = reserved_slot;

    return reserved_slot;
  } 

}
