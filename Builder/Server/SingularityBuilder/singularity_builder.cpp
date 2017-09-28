#include <iostream>
#include <string>
#include <sstream>
#include <errno.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <csignal>
#include <system_error>
#include "sqlite3.h"

// File namespace(static)
namespace {
  constexpr auto gDatabase = "Builder";
  volatile std::sig_atomic_t gShouldKill = 0;

  void signal_handler(int signal) {
    gShouldKill = 1;
  }

  // Stop the named docker container
  void docker_stop(const std::string& instance_name) {
    std::string stop_command;
    stop_command += "docker stop " + instance_name;
    boost::process::system(stop_command);
  }

  // Execute a docker run command
  // Docker will ignore normal signals
  // docker stop {container_name} must be used
  int docker_run(const std::string& command, const std::string& instance_name) {
    namespace bp = boost::process;
    int return_code;

    // Launch the command asynchronously
    bp::child docker_proc(command);

    // Test if we should stop docker
    // This can be set by signal handlers
    while(docker_proc.running()) {
      if(gShouldKill) {
        docker_stop(instance_name);
      }
    }

    // Wait for docker to complete
    docker_proc.wait();
    return_code = docker_proc.exit_code();

    return return_code;
  }
  
  // Enter a new job into the queue
  void enter_queue(sqlite3* db, const std::string& job_id) {
    char *db_err = NULL;
    std::string SQL_command;
    SQL_command += "INSERT INTO build_queue(job_id) VALUES(" + job_id + ");";
    int rc = sqlite3_exec(db, SQL_command.c_str(), NULL, NULL, &db_err);
    if(rc != SQLITE_OK) {
      std::string err(db_err);
      sqlite3_free(db_err);
      throw std::system_error(ECONNABORTED, std::generic_category(), err);
    }
  }

  // Remove a job from the queue
  void exit_queue(sqlite3 *db, const std::string& job_id) {
    char *db_err = NULL;
    std::string SQL_command;
    SQL_command += "DELETE FROM build_queue WHERE job_id = " + job_id + ");";
    int rc = sqlite3_exec(db, SQL_command.c_str(), NULL, NULL, &db_err);
    if(rc != SQLITE_OK) {
      std::string err(db_err);
      sqlite3_free(db_err);
      throw std::system_error(ECONNABORTED, std::generic_category(), err);
    }
  }

  // Return true if the specified job is at the top of the queue
  int first_in_queue_callback(void *first_in_queue, int count, char** values, char** names) {
    *static_cast<std::string*>(first_in_queue) = values[0];
    return 0;
  }
  bool first_in_queue(sqlite3 *db, const std::string& job_id) {
    char *db_err = NULL;
    std::string first_in_queue;
    std::string SQL_command;
    SQL_command += "SELECT job_id FROM build_queue ORDER BY id ASC LIMIT 1;";
    int rc = sqlite3_exec(db, SQL_command.c_str(), first_in_queue_callback, &first_in_queue, &db_err);
    if(rc != SQLITE_OK) {
      std::string err(db_err);
      sqlite3_free(db_err);
      throw std::system_error(ECONNABORTED, std::generic_category(), err);
    }
    return first_in_queue == job_id;
  }

  // Reserve a valid loop device id if possible, or -1 if no device is available
  int reserve_loop_id_callback(void *loop_id, int count, char** values, char** names) {
    *static_cast<int*>(loop_id) = std::stoi(std::string(values[0]));

    return 0;
  }
  int reserve_loop_id(sqlite3 *db) {
    char *db_err = NULL;
    int loop_id;

    // Begin transaction
    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    // Retrive a loop device id if possible
    std::string SQL_fetch;
    SQL_fetch = "SELECT loop_id FROM available_loops LIMIT 1;";
    int rc = sqlite3_exec(db, SQL_fetch.c_str(), reserve_loop_id_callback, &loop_id, &db_err);
    if(rc != SQLITE_OK) {
      std::string err(db_err);
      sqlite3_free(db_err);
      throw std::system_error(ECONNABORTED, std::generic_category(), err);
    }

    // Remove the loop device id from available devices
    if(loop_id >= 0) {
      std::string SQL_remove;
      SQL_remove = "DELETE FROM available_loops WHERE loop_id = " + std::to_string(loop_id) + ");";
      int rc = sqlite3_exec(db, SQL_remove.c_str(), NULL, NULL, &db_err);
      if(rc != SQLITE_OK) {
        std::string err(db_err);
        sqlite3_free(db_err);
        throw std::system_error(ECONNABORTED, std::generic_category(), err);
      }
    }

    // End transaction
    sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);

    return loop_id;
  }

  // Release a loop id back to the pool of available devices
  void release_loop_id(sqlite3 *db, int loop_id) {
    char *db_err = NULL;
    std::string SQL_command;
    SQL_command += "INSERT INTO available_loops(loop_id) VALUES(" + std::to_string(loop_id) + ");";
    int rc = sqlite3_exec(db, SQL_command.c_str(), reserve_loop_id_callback, &loop_id, &db_err);
    if(rc != SQLITE_OK) {
      std::string err(db_err);
      sqlite3_free(db_err);
      throw std::system_error(ECONNABORTED, std::generic_category(), err);
    }
  }

  // Initialize the build process
  void builder_init(sqlite3 **db) {
    int db_rc;
    db_rc = sqlite3_open(gDatabase, db);
    if(db_rc != SQLITE_OK) {
      throw std::system_error(ECONNABORTED, std::generic_category(), sqlite3_errmsg(*db));
    }
  }

  // Cleanup the build process
  void builder_cleanup(sqlite3 *db, std::string job_id, std::string docker_name, int loop_id) {
    // Remove job from queue
    exit_queue(db, job_id);

    // Add loop device back to pool
    if(loop_id >= 0)
      release_loop_id(db, loop_id);

    // Close database
    sqlite3_close(db);
  }
} // End anonymous namespace

int main(int argc, char** argv) {  
  // SingularityBuilder requires two arguments, a uniqueID string and a work path string
  if(argc != 3) {
    std::cerr<<"ERROR: SingularityBuilder: Invalid argument count\n";
    return EXIT_FAILURE;
  }

  // Create string from char* argument
  std::string job_id(argv[1]);
  std::string work_path(argv[2]);

  // Register signal handlers
  std::signal(SIGABRT, signal_handler);
  std::signal(SIGBUS,  signal_handler);
  std::signal(SIGHUP,  signal_handler);
  std::signal(SIGILL,  signal_handler);
  std::signal(SIGINT,  signal_handler);
  std::signal(SIGPIPE, signal_handler);
  std::signal(SIGQUIT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  int err;
  sqlite3 *db = NULL; 
  int loop_id = -1;

  try {
    // Establish connection to database
    builder_init(&db);

    // Enter job request into queue
    enter_queue(db, job_id);
  
    // Wait in queue for a loop device to become available
    while(loop_id < 0) {
      if(first_in_queue(db, job_id)) {
        loop_id = reserve_loop_id(db);
        if(loop_id >= 0)
          exit_queue(db, job_id);
      }
    }   

    // Once a loop device is available execute the build command
    std::string loop_device;
    loop_device += "/dev/loop" + std::to_string(loop_id);
    std::string builder_command;
    builder_command += "docker run --device=" + loop_device + " --security-opt apparmor=docker-singularity --cap-add SYS_ADMIN --name "
                       + job_id + " -v " + work_path + ":/work_dir -w /work_dir singularity_builder";
    err = docker_run(builder_command, job_id);

  } catch(const std::system_error& error) {
      std::cerr << "ERROR: " << error.code() << " - " << error.what() << std::endl;
      err = error.code().value();
  } catch(const std::exception &error) {
      std::cerr<<"ERROR: " << error.what() << std::endl;
      err = EXIT_FAILURE;
  } catch(...) {
      std::cerr << "ERROR: Unknown" << std::endl;
      err = EXIT_FAILURE;
  }

  builder_cleanup(db, job_id, job_id, loop_id);

  return err;
}
