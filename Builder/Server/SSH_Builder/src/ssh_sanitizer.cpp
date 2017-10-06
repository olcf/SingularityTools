#include "ssh_sanitizer.h"
#include <iostream>
#include <string>
#include <sstream>
#include <errno.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <system_error>
#include "singularity_builder.h"
#include "signal_handler.h"

// Execute a sanitized copy of $SSH_ORIGINAL_COMMAND, as provided by the ssh authorized_keys command option
// A single string argument is expected so it must be quoted when setting command, e.g.
// command="/path/to/CommandSanitizer \"${SSH_ORIGINAL_COMMAND}\"" ssh-rsa ...
// the SSH_CONNECTION info is used as a unique identifier to ensure users only have very limited access

// The following commands are allowed:
// scp -t container.def
// scp -f container.img
// BuilderPrep
// BuilderRun
// BuilderCleanup

// Note that SCP is a two way command and once it's finished on the initiating side it will return
// So we can't get fancy and have the SCP call do the prep/build/cleanup

namespace builder {
  // Compile time constants
  constexpr auto gScpTo = "scp -t container.def";
  constexpr auto gScpFrom = "scp -f container.img"
  constexpr auto gBuilderWorkPath = "/home/builder/container_scratch/";
  constexpr auto gBuilderPrep = "BuilderPrep";
  constexpr auto gBuilderRun = "BuilderRun";
  constexpr auto gBuilderCleanup = "BuilderCleanup";

   // Sanitize the command to be run
  static std::string get_command(int argc, char **argv) {
    // A single string argument is required
    if(argc != 2) {
      throw std::system_error(EINVAL, std::generic_category(), "Invalid number of SSH_Builder arguments");
    }

    // Create string from char* argument
    std::string command(argv[1]);

    // Quick sanity check for any invalid special characters
    std::string invalid_chars("!%^*~|;(){}[]$#\\");
    for(char c : invalid_chars) {
      if(command.find(c) != std::string::npos) {
        throw std::system_error(EINVAL, std::generic_category(), "invalid SSH_Builder argument characters");
      }
    }

    // Remove any leading/trailing white space
    boost::trim(command);

    return command;
  }


  // Get a unique ID based upon SSH_CONNECTION information
  // This allows us to keep track of a user who is making multiple SSH calls using control master
  static std::string get_unique_id() {
    char const* tmp = getenv("SSH_CONNECTION");
    if ( tmp == NULL ) {
      throw std::system_error(EIDRM, std::generic_category(), "SSH_CONNECTION");
    }

    std::string SSH_CONNECTION(tmp);

    // Remove any leading/trailing white space
    boost::trim(SSH_CONNECTION);

    // Test for the correct number of components
    std::vector<std::string> split_connection;
    boost::split(split_connection, SSH_CONNECTION, boost::is_any_of("\t "), boost::token_compress_on);
    if(split_connection.size() != 4) {
      throw std::system_error(EINVAL, std::generic_category(), "SSH_CONNECTION");
    }

    // Replace the spaces with underscores
    boost::replace_all(SSH_CONNECTION, " ", "_");

    return SSH_CONNECTION;
  }

  // Use the unique_id to generate a full unique work directory path based in gBuilderWorkPath
  static std::string get_unique_work_path() {
    std::string work_path;
    work_path += gBuilderWorkPath + get_unique_id();
    return work_path;
  }

  SSH_Sanitizer::SSH_Sanitizer(int argc, char**argv) : unique_id(get_unique_id()),
                                                       unique_work_path(get_unique_work_path()),
                                                       command(get_command(argc, argv))
  {
    // Register signal handlers
    std::signal(SIGABRT, signal_handler);
    std::signal(SIGBUS,  signal_handler);
    std::signal(SIGHUP,  signal_handler);
    std::signal(SIGILL,  signal_handler);
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGPIPE, signal_handler);
    std::signal(SIGQUIT, signal_handler);
    std::signal(SIGTERM, signal_handler);
  }

  int SSH_Sanitizer::blocking_exec(std::string command) {
    namespace bp = boost::process;

    bp::environment env;   // Blank environment env
    int return_code;

    // In debug mode simply print the statement to be executed
    #ifdef DEBUG
      std::cout<<command;
      return 0;
    #endif

    // Launch the command asynchronously
    bp::child child_proc(command, env);

    // Test if we should terminate the command
    // This can be set by signal handlers
    while(child_proc.running()) {
      if(gShouldKill) {
        pid_t pid = child_proc.id();
        kill(pid, SIGINT);
      }
    }

    // Wait for child to complete
    child_proc.wait();
    return_code = child_proc.exit_code();

    return return_code;
  }

  // Create the unique working directory where the definition and final image will be stored
  int SSH_Sanitizer::run_builder_prep() {
    #ifdef DEBUG
    std::cout<<"mkdir " << this->unique_work_path;
    return 0;
    #endif

    boost::filesystem::create_directories(this->unique_work_path);
    return 0;
  }

  // Build the container
  int SSH_Sanitizer::run_builder() {
    #ifdef DEBUG
    std::cout<<"builder.build()";
    return 0;
    #endif

    SingularityBuilder builder(this->unique_work_path, this->unique_id);
    int err = builder.build();
    return err;
  }

  // Remove the unique working directory
  int SSH_Sanitizer::run_builder_cleanup() {
    #ifdef DEBUG
    std::cout<<"rm " << this->unique_work_path;
    return 0;
    #endif

    boost::filesystem::remove_all(this->unique_work_path);
    return 0;
  }

  // We allow exactly two scp cases, the command passed to SSH_ORIGINAL_COMMAND is NOT the same is is run on the client
  // The client generally doesn't know the actual path to the container so the command is transformed as follows
  // scp -t container.def -> scp -t unique_work_path/container.def
  // scp -f container.img -> scp -f unique_work_path/container.img
  // Upon transfering the definition to the builder we create the unique directory and kick off the build process
  // Upon transfering the final image to the client we delete the unique work directory o nthe builder 
  int SSH_Sanitizer::run_scp_to() {
    // Initiate SCP "to" the builder
    std::string scp_call;
    scp_call += "scp -t " + unique_work_path + "/container.def";
    int err = blocking_exec(scp_call);
    return err;
  }
  int SSH_Sanitizer::run_scp_from() {
    // Initiate SCP "from" the builder
    std::string scp_call;
    scp_call += "scp -f " + unique_work_path + "/container.img";
    int err = blocking_exec(scp_call);
    return err;
  }


  // Attempt to run provided command if it's allowed
  int SSH_Sanitizer::sanitized_run() {
    int err;

    std::string base_command = command[0];

    if(base_command == gScpTo){
      err = run_scp_to();
    }
    else if(base_command == gScpFrom){
      err = run_scp_from();
    }
    else if(base_command == gBuilderPrep) {
      err = run_builder_prep();
    }
    else if(base_command == gBuilderRun) {
      err = run_builder();
    }
    else if(base_command == gBuilderCleanup) {
      err = run_builder_cleanup();
    }
    else {
      throw std::system_error(ENOSYS, std::generic_category(), "SSH_Sanitizer");
    }
    return err;
  }
}
