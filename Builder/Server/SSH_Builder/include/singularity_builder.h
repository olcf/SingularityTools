#pragma once

#include <string>
#include "sqlite3.h"

namespace builder {
  class SingularityBuilder {

  public:
    // Constructors
    SingularityBuilder(std::string work_path, std::string build_id);
    ~SingularityBuilder();
    SingularityBuilder()                                     = delete;
    SingularityBuilder(const SingularityBuilder&)            = delete;
    SingularityBuilder& operator=(const SingularityBuilder&) = delete;
    SingularityBuilder(SingularityBuilder&&) noexcept        = delete;
    SingularityBuilder& operator=(SingularityBuilder&&)      = delete;

    // Start the build process
    int build();

  private:
    int vagrant_build();
    int vagrant_up();
    void enter_queue();
    void exit_queue(bool should_throw);
    bool first_in_queue();
    bool reserve_build_spot();
    void release_build_spot(bool should_throw);

    sqlite3 *db;
    bool has_build_spot;
    const std::string work_path;
    const std::string build_id;
  };
}
