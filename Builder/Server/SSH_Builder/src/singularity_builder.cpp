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

// TODO: MAKE THIS SET IN BUILD
#define MAX_VM_COUNT 4

namespace builder {
  constexpr bool NO_THROW = false;
  constexpr auto gDatabase = "/home/builder/Builder.db";
 
   namespace bp = boost::process;

  // Stop the vagrant VM and remove it
  static void vagrant_destroy() {
    std::cerr<<"Attempting to destroy VM..."<<std::endl;
    std::string stop_command("vagrant destroy");
    boost::process::system(stop_command);
  }

  // Fire up the VM
  int SingularityBuilder::vagrant_up() {
    std::string vagrant_up_command;
    vagrant_up_command += "vagrant up";

    // Launch the command asynchronously
    bp::child vagrant_proc(vagrant_up_command);

    // Test if we should stop vagrant
    while(vagrant_proc.running()) {
      if(gShouldKill) {
        vagrant_destroy();
      }
    }

    // Wait for vagrant to complete bool
    vagrant_proc.wait();
    return vagrant_proc.exit_code();
  }

  // Run singularity build within our vagrant container
  int SingularityBuilder::vagrant_build() {
    std::string vagrant_build_command("vagrant ssh -c 'sudo singularity build /vagrant/container.img /vagrant/container.def'");

    // Launch the command asynchronously
    bp::child vagrant_proc_build(vagrant_build_command);

    // Test if we should stop vagrant
    while(vagrant_proc_build.running()) {
      if(gShouldKill) {
        vagrant_destroy();
      }
    }

    // Wait for vagrant to complete bool
    vagrant_proc_build.wait();
    return vagrant_proc_build.exit_code();
  }
  
  // Enter a new build into the queue
  void SingularityBuilder::enter_queue() {
    char *db_err = NULL;
    std::string SQL_command;
    SQL_command += "INSERT INTO build_queue(build_id) VALUES(" + this->build_id + ");";
    int rc = sqlite3_exec(this->db, SQL_command.c_str(), NULL, NULL, &db_err);
    if(rc != SQLITE_OK) {
      std::string err(db_err);
      sqlite3_free(db_err);
      throw std::system_error(ECONNABORTED, std::generic_category(), err);
    }
  }

  // Remove a build from the queue
  void SingularityBuilder::exit_queue(bool should_throw=true) {
    char *db_err = NULL;
    std::string SQL_command;
    SQL_command += "DELETE FROM build_queue WHERE build_id = " + this->build_id + ";";
    int rc = sqlite3_exec(this->db, SQL_command.c_str(), NULL, NULL, &db_err);
    if(rc != SQLITE_OK && should_throw) {
      std::string err(db_err);
      sqlite3_free(db_err);
      throw std::system_error(ECONNABORTED, std::generic_category(), err);
    }
  }

  // Return true if the specified build is at the top of the queue
  static int first_in_queue_callback(void *first_in_queue, int count, char** values, char** names) {
    *static_cast<std::string*>(first_in_queue) = values[0];
    return 0;
  }
  bool SingularityBuilder::first_in_queue() {
    char *db_err = NULL;
    std::string first_in_queue;
    std::string SQL_command;
    SQL_command += "SELECT build_id FROM build_queue ORDER BY id ASC LIMIT 1;";
    int rc = sqlite3_exec(this->db, SQL_command.c_str(), first_in_queue_callback, &first_in_queue, &db_err);
    if(rc != SQLITE_OK) {
      std::string err(db_err);
      sqlite3_free(db_err);
      throw std::system_error(ECONNABORTED, std::generic_category(), err);
    }
    return first_in_queue == build_id;
  }

  // If active VM count < MAX_VM_COUNT incriment VM count and return true
  // Reserve a valid loop device id if possible, or -1 if no device is available
  static int active_count_callback(void *active_vm_count, int count, char** values, char** names) {
    *static_cast<int*>(active_vm_count) = std::stoi(std::string(values[0]));
    return 0;
  }

  // Decrement the number of active build
  void SingularityBuilder::release_build_spot(bool should_throw=true) {
    char *db_err = NULL;

    std::string SQL_insert;
    SQL_insert += "UPDATE active_builds SET count = count - 1 WHERE id = 1;";
    int rc = sqlite3_exec(this->db, SQL_insert.c_str(), NULL, NULL, &db_err);
    if(rc != SQLITE_OK && should_throw) {
      std::string err(db_err);
      sqlite3_free(db_err);
      throw std::system_error(ECONNABORTED, std::generic_category(), err);
    }

    // End transaction
    sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
  }

  // Reserve a build spot if one is available, return true if reserved else return false
  bool SingularityBuilder::reserve_build_spot() {
    char *db_err = NULL;

    // Begin transaction
    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    // Read the number of active VMs
    int active_vm_count;
    std::string SQL_fetch("SELECT count FROM active_builds LIMIT 1;");
    int rc = sqlite3_exec(this->db, SQL_fetch.c_str(), active_count_callback, &active_vm_count, &db_err);
    if(rc != SQLITE_OK) {
      std::string err(db_err);
      sqlite3_free(db_err);
      throw std::system_error(ECONNABORTED, std::generic_category(), err);
    }

    // If there is space reserve a spot
    bool reserved_spot = false;
    if(active_vm_count < MAX_VM_COUNT) {
      active_vm_count++;
      std::string SQL_insert;
      SQL_insert += "UPDATE active_builds set count = " + std::to_string(active_vm_count) + " WHERE id = 1;";
      int rc = sqlite3_exec(this->db, SQL_insert.c_str(), NULL, NULL, &db_err);
      if(rc != SQLITE_OK) {
        std::string err(db_err);
        sqlite3_free(db_err);
        throw std::system_error(ECONNABORTED, std::generic_category(), err);
      }
      reserved_spot = true;
    }
    // End transaction
    sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);

    // Update objects build spot status
    this->has_build_spot = reserved_spot;
  }

  // Open the database
  static void db_init(sqlite3** db) {
    int db_rc;
    db_rc = sqlite3_open_v2(gDatabase, db, SQLITE_OPEN_READWRITE, NULL);
    if(db_rc != SQLITE_OK) {
      throw std::system_error(ECONNABORTED, std::generic_category(), sqlite3_errmsg(*db));
    }
  }

  SingularityBuilder::SingularityBuilder(std::string work_path, std::string build_id) : has_build_spot{false},
                                                                                        work_path{work_path},
                                                                                        build_id{build_id}
  {
    db_init(&(this->db));
  }

  SingularityBuilder::~SingularityBuilder() {
    // Remove build from queue
    // We probably could only run this if the build is infact queued
    this->exit_queue(NO_THROW);

    // Release the build spot if one has been reserved
    if(this->has_build_spot)
      release_build_spot(NO_THROW);

    // Remove the vagrant vm
    vagrant_destroy();

    // Close database
    sqlite3_close(db);
  }

  int SingularityBuilder::build() {
    // Enter build request into queue
    enter_queue();

    // Wait in queue for a build spot to open up
    while(!this->has_build_spot) {
      if(this->first_in_queue()) {
        this->reserve_build_spot();
        if(this->has_build_spot)
          this->exit_queue();
      }
    }

    // Once a build spot is available spin up the vm and execute the build command
    int err = this->vagrant_up();
    if(!err)
      err = this->vagrant_build();

    return err;
  }
}
