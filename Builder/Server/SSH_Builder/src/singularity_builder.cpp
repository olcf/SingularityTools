#include "singularity_builder.h"
#include <iostream>
#include <string>
#include <sstream>
#include <errno.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <system_error>
#include "sqlite3.h"
#include "signal_handler.h"

namespace builder {
  constexpr bool NO_THROW = false;
  constexpr auto gDatabase = "Builder";

  // Stop the docker container
  // Note: docker rm should be called once the container is stopped
  // This command is blocking
  void SingularityBuilder::stop_docker_build() {
    std::cerr<<"Attempting to kill docker..."<<std::endl;
    std::string stop_command;
    stop_command += "docker stop " + this->docker_container_name;
    boost::process::system(stop_command);
  }

  // Execute a docker run command
  // Docker has wonky signal handling so
  // docker stop {container_name} must be used
  int SingularityBuilder::docker_build() {
    namespace bp = boost::process;

    std::string docker_command;
    docker_command += "docker run --device=" + this->loop_device() + " --security-opt apparmor=docker-singularity --cap-add SYS_ADMIN --name "
                       + this->docker_container_name + " -mount type=bind,source=" + this->work_path
                       + ",destination=/work_dir -w /work_dir singularity_builder";

    // Launch the command asynchronously
    bp::child docker_proc(docker_command);

    // Test if we should stop docker
    while(docker_proc.running()) {
      if(gShouldKill) {
        this->stop_docker_build();
      }
    }

    // Wait for docker to complete
    docker_proc.wait();
    int return_code = docker_proc.exit_code();

    return return_code;
  }
  
  // Enter a new job into the queue
  void SingularityBuilder::enter_queue() {
    char *db_err = NULL;
    std::string SQL_command;
    SQL_command += "INSERT INTO build_queue(job_id) VALUES(" + this->job_id + ");";
    int rc = sqlite3_exec(this->db, SQL_command.c_str(), NULL, NULL, &db_err);
    if(rc != SQLITE_OK) {
      std::string err(db_err);
      sqlite3_free(db_err);
      throw std::system_error(ECONNABORTED, std::generic_category(), err);
    }
  }

  // Remove a job from the queue
  void SingularityBuilder::exit_queue(bool should_throw=true) {
    char *db_err = NULL;
    std::string SQL_command;
    SQL_command += "DELETE FROM build_queue WHERE job_id = " + this->job_id + ";";
    int rc = sqlite3_exec(this->db, SQL_command.c_str(), NULL, NULL, &db_err);
    if(rc != SQLITE_OK && should_throw) {
      std::string err(db_err);
      sqlite3_free(db_err);
      throw std::system_error(ECONNABORTED, std::generic_category(), err);
    }
  }

  // Return true if the specified job is at the top of the queue
  static int first_in_queue_callback(void *first_in_queue, int count, char** values, char** names) {
    *static_cast<std::string*>(first_in_queue) = values[0];
    return 0;
  }
  bool SingularityBuilder::first_in_queue() {
    char *db_err = NULL;
    std::string first_in_queue;
    std::string SQL_command;
    SQL_command += "SELECT job_id FROM build_queue ORDER BY id ASC LIMIT 1;";
    int rc = sqlite3_exec(this->db, SQL_command.c_str(), first_in_queue_callback, &first_in_queue, &db_err);
    if(rc != SQLITE_OK) {
      std::string err(db_err);
      sqlite3_free(db_err);
      throw std::system_error(ECONNABORTED, std::generic_category(), err);
    }
    return first_in_queue == job_id;
  }

  // Reserve a valid loop device id if possible, or -1 if no device is available
  static int reserve_loop_id_callback(void *loop_id, int count, char** values, char** names) {
    *static_cast<int*>(loop_id) = std::stoi(std::string(values[0]));

    return 0;
  }
  void SingularityBuilder::reserve_loop_id() {
    char *db_err = NULL;

    // Begin transaction
    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    // Retrive a loop device id if possible
    std::string SQL_fetch;
    SQL_fetch = "SELECT loop_id FROM available_loops LIMIT 1;";
    int rc = sqlite3_exec(this->db, SQL_fetch.c_str(), reserve_loop_id_callback, &(this->loop_id), &db_err);
    if(rc != SQLITE_OK) {
      std::string err(db_err);
      sqlite3_free(db_err);
      throw std::system_error(ECONNABORTED, std::generic_category(), err);
    }

    // Remove the loop device id from available devices
    if(this->loop_id_valid()) {
      std::string SQL_remove;
      SQL_remove = "DELETE FROM available_loops WHERE loop_id = " + std::to_string(loop_id) + ";";
      int rc = sqlite3_exec(this->db, SQL_remove.c_str(), NULL, NULL, &db_err);
      if(rc != SQLITE_OK) {
        std::string err(db_err);
        sqlite3_free(db_err);
        throw std::system_error(ECONNABORTED, std::generic_category(), err);
      }
    }

    // End transaction
    sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
  }

  // Release a loop id back to the pool of available devices
  void SingularityBuilder::release_loop_id(bool should_throw=true) {
    char *db_err = NULL;
    std::string SQL_command;
    SQL_command += "INSERT INTO available_loops(loop_id) VALUES(" + std::to_string(this->loop_id) + ");";
    int rc = sqlite3_exec(this->db, SQL_command.c_str(), reserve_loop_id_callback, &loop_id, &db_err);
    if(rc != SQLITE_OK && should_throw) {
      std::string err(db_err);
      sqlite3_free(db_err);
      throw std::system_error(ECONNABORTED, std::generic_category(), err);
    }
  }

  void SingularityBuilder::remove_docker_container() {
    std::string rm_command;
    rm_command += "docker rm " + this->docker_container_name;
    boost::process::system(rm_command);
  }

  // Open the database
  static void db_init(sqlite3** db) {
    int db_rc;
    db_rc = sqlite3_open(gDatabase, db);
    if(db_rc != SQLITE_OK) {
      throw std::system_error(ECONNABORTED, std::generic_category(), sqlite3_errmsg(*db));
    }
  }

  SingularityBuilder::SingularityBuilder(std::string work_path,
                                         std::string docker_container_name) : loop_id{-1},
                                                                              work_path{work_path},
                                                                              docker_container_name{docker_container_name}
  {
    db_init(&(this->db));
  }

  SingularityBuilder::~SingularityBuilder() {
    // Remove job from queue
    this->exit_queue(NO_THROW);

    // Add loop device back to pool
    if(this->loop_id_valid())
      this->release_loop_id(NO_THROW);

    // Remove the docker container
    this->remove_docker_container();

    // Close database
    sqlite3_close(db);
  }

  bool SingularityBuilder::loop_id_valid() {
    return this->loop_id >= 0;
  }

  std::string SingularityBuilder::loop_device() {
    if(this->loop_id_valid()) {
      std::string loop_device;
      loop_device += "/dev/loop" + std::to_string(this->loop_id);
      return loop_device;
    } else {
        throw std::system_error(ENXIO, std::generic_category(), "No valid loop device reserved");
    }
  }

  int SingularityBuilder::build() {
    // Enter job request into queue
    enter_queue();

    // Wait in queue for a loop device to become available
    while(!this->loop_id_valid()) {
      if(this->first_in_queue()) {
        this->reserve_loop_id();
        if(this->loop_id_valid())
          this->exit_queue();
      }
    }

    // Once a loop device is available execute the build command
    int err = this->docker_build();
    return err;
  }
}
