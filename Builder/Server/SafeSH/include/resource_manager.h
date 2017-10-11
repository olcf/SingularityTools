#pragma once
  
#include <string>
#include "sql_db.h"

namespace builder {
  class ResourceManager {

  public:
    // Constructors
    ResourceManager();
    ~ResourceManager()                                    = default;
    ResourceManager(const ResourceManager&)               = delete;
    ResourceManager& operator=(const ResourceManager&)    = delete;
    ResourceManager(ResourceManager&&) noexcept           = delete;
    ResourceManager& operator=(ResourceManager&&)         = delete;

    bool reserve_build_slot();
    void release_build_slot();
    bool build_slot_reserved;

  private:
    SQL db;
  };
}
