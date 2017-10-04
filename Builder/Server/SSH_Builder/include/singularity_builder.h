#pragma once

#include <string>
#include "sqlite3.h"

namespace builder {
  class SingularityBuilder {

  public:
    // Constructors
    SingularityBuilder(std::string work_path, std::string docker_container_name);
    ~SingularityBuilder();
    SingularityBuilder()                                     = delete;
    SingularityBuilder(const SingularityBuilder&)            = delete;
    SingularityBuilder& operator=(const SingularityBuilder&) = delete;
    SingularityBuilder(SingularityBuilder&&) noexcept        = delete;
    SingularityBuilder& operator=(SingularityBuilder&&)      = delete;

    // Start the build process
    int build();

  private:
    int docker_build();
    void stop_docker_build();
    void enter_queue();
    void exit_queue(bool should_throw);
    bool first_in_queue();
    void reserve_loop_id();
    void release_loop_id(bool should_throw);
    bool loop_id_valid();
    std::string loop_device();
    void remove_docker_container();

    sqlite3 *db;
    int loop_id;
    const std::string work_path;
    const std::string job_id;
    const std::string docker_container_name;
  };
}
