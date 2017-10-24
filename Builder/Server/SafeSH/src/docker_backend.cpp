#include "docker_backend.h"
#include <iostream>
#include <string>
#include <system_error>
#include <cerrno>
#include "sql_db.h"
#include "signal_handler.h"
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
#include "build_queue.h"
#include "docker_backend.h"

namespace builder {
#ifndef ENONET
#define ENONET 2
#endif
  namespace bp = boost::process;

  DockerBackend::~DockerBackend() {
    this->tear_down();
  }

  // Stop the docker instance and remove it
  void DockerBackend::tear_down() {
    std::cerr << "Attempting to kill docker..." << std::endl;
    std::string stop_command;
    stop_command += "docker stop " + this->docker_name;
    this->docker_name = "";
    bp::system(stop_command);

      std::string rm_command;
      rm_command += "docker rm " + this->docker_name;
      boost::process::system(rm_command);
  }

  // Run singularity build within docker instance
  int DockerBackend::build() {
    BuildQueue queue;

    int rc = queue.run([&](std::string slot_id) {
      // The reserved loop device is implicitly set to be the corresponding slot id from the build queue reservation
      std::string loop_device;
      loop_device = "/dev/loop" + slot_id;

      // Set a unique name for this docker container instance based upon the unique loop id assigned
      this->docker_name = "docker_on_loop_" + slot_id;

      std::string working_directory = boost::filesystem::current_path().string();
      std::string docker_command;
      docker_command += "docker run --device=" + loop_device +
                        " --security-opt apparmor=docker-singularity --cap-add SYS_ADMIN --name "
                        + this->docker_name + " --mount type=bind,source=" + working_directory
                        + ",destination=/work_dir -w /work_dir singularity_builder";

      // Launch the command asynchronously
      bp::child docker_proc(docker_command);

      // Test if we should stop docker
      while (docker_proc.running()) {
        if (gShouldKill) {
          this->tear_down();
        }
      }

      // Wait for docker to complete
      docker_proc.wait();
      int return_code = docker_proc.exit_code();
      return return_code;
    });

    return rc;
  }

}
