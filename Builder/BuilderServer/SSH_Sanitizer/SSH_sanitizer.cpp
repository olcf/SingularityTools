#include <iostream>
#include <string>
#include <sstream>
#include <errno.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <csignal>
#include <system_error>

// Execute a sanitized copy of $SSH_ORIGINAL_COMMAND, as provided by the ssh authorized_keys command option
// A single string argument is expected so it must be quoted when setting command, e.g.
// command="/path/to/CommandSanitizer \"${SSH_ORIGINAL_COMMAND}\"" ssh-rsa ...
// the SSH_CONNECTION info is used as a unique identifier to ensure users only have very limited access

// The following commands are allowed:
// /usr/local/bin/SingularityBuilder container_size
// /usr/bin/scp -t local/file cades@{BUILDER_IP}:{BUILDER_PATH}/file
// /usr/bin/scp -t cades@{BUILDER_IP}:{BUILDER_PATH}/file local/file
// GetWorkPath

// File namespace(static)
namespace {
  constexpr auto gBuilderBase = "/usr/local/bin/SingularityBuilder";
  constexpr auto gScpBase = "/usr/bin/scp";
  constexpr auto gBuilderDirectoryPath = "/home/builder/container_scratch/";
  constexpr auto gGetWorkPath = "GetWorkPath";

  volatile std::sig_atomic_t gShouldKill = 0;

  void signal_handler(int signal) {
    gShouldKill = 1;
  }

  // Preform a blocking exec
  int blocking_exec(std::string command) {
    namespace bp = boost::process;

    bp::group child_group; // Allow child processes of command to be terminated
    bp::environment env;   // Blank environment env

    int return_code;

    // Launch the command asynchronously
    bp::child child_proc(command, child_group, env, bp::std_in.close());
    // Test if we should terminate the command
    // This can be set by signal handlers
    while(child_proc.running()) {
      if(gShouldKill) {
        child_group.terminate();
      }
    }

    // Wait for child to complete
    child_proc.wait();
    return_code = child_proc.exit_code();

    return return_code;
  }

  // Retrive the build host IP from SSH_CONNECTION env var
  std::string builder_ip() {
    char const* tmp = getenv("SSH_CONNECTION");
    if ( tmp == NULL ) {
      throw std::system_error(EIDRM, std::generic_category(), "SSH_CONNECTION");
    }

    std::string SSH_CONNECTION(tmp);

    // Remove any leading/trailing white space
    boost::trim(SSH_CONNECTION);

    std::vector<std::string> split_connection;
    boost::split(split_connection, SSH_CONNECTION, boost::is_any_of("\t "), boost::token_compress_on);
    if(split_connection.size() != 4) {
      throw std::system_error(EINVAL, std::generic_category(), "SSH_CONNECTION");
    }
    return split_connection[2];
  }

  // Get the temporary unique build directory, based on concating the SSH_CONNECTION env var
  std::string unique_work_path() {
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

    return gBuilderDirectoryPath + SSH_CONNECTION;
  }

  void builder_prep() {
    boost::filesystem::create_directories(unique_work_path());
  }

  void builder_cleanup() {
    boost::filesystem::remove_all(unique_work_path());
  }


  int run_SingularityBuilder(const std::vector<std::string>& split_command) {
    if(split_command.size() != 2) {
      throw std::system_error(EINVAL, std::generic_category(), "SingularityBuilder");
    }
    const std::string& container_size = split_command[1];

    std::string builder_call{gBuilderBase};
    builder_call += " " + container_size + " " + unique_work_path() + " " + unique_work_path();        
    int err = blocking_exec(builder_call);
    return err;
  }

  // We allow exactly two scp cases
  // scp /arbitrary/file.def builder@{builder_IP}:unique_work_path()/container.def
  // scp builder@{builder_IP}:unique_work_path()/container.img /arbitrary/file.img
  // Upon transfering the definition we create the unique directory and upon transfering the final image we delete it 
  int run_scp(const std::vector<std::string>& split_command) {
    // Check number of arguments
    if(split_command.size() != 4) {
      throw std::system_error(EINVAL, std::generic_category(), "scp");
    }

    // Check for "-t" flag, this is a hidden flag present on the remote end of the SCP call
    if(split_command[1] != "-t") {
      throw std::system_error(EINVAL, std::generic_category(), "scp: -t");
    }

    const std::string& source = split_command[2];
    const std::string& destination = split_command[3];

    // Generate allowable builder targets
    std::string definition_target;
    definition_target += "builder@" + builder_ip() + ":" + unique_work_path() + "/container.def";
    std::string container_target;
    container_target += "builder@" + builder_ip() + ":" + unique_work_path() + "/container.img";

    std::string scp_call{gScpBase};
    scp_call += " -t " + source + " " + destination;

    int err;
    // Check for allowed cases
    if(destination == definition_target && source.find(":") == std::string::npos) {
      builder_prep();
      err = blocking_exec(scp_call);
    } 
    else if(source == container_target && destination.find(":") == std::string::npos) {
      err = blocking_exec(scp_call);
      builder_cleanup();
    } 
    else {
      throw std::system_error(EINVAL, std::generic_category(), "scp");
    }

    return err;
  }

} // End anonymous namespace

int main(int argc, char** argv) {  
  // A single string argument is required
  if(argc != 2) {
    throw std::system_error(EINVAL, std::generic_category(), "SSH_Sanitizer");
  }

  // Create string from char* argument
  std::string command(argv[1]);

  // Quick sanity check for any invalid special characters
  std::string invalid_chars("!%^*~|;(){}[]$#\\");
  for(char c : invalid_chars) {
    if(command.find(c) != std::string::npos) {
      throw std::system_error(EINVAL, std::generic_category(), "SSH_CONNECTION");
    }
  }

  // Remove any leading/trailing white space
  boost::trim(command);

  // Split the string command on space(s) or tab(s)
  std::vector<std::string> split_command;
  boost::split(split_command, command, boost::is_any_of("\t "), boost::token_compress_on);

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

  try {
    if(split_command[0] == gBuilderBase) {
      err = run_SingularityBuilder(split_command);
    }
    else if(split_command[0] == gScpBase){
      err = run_scp(split_command);
    }
    else if(split_command[0] == gGetWorkPath) {
      std::cout<<unique_work_path()<<std::endl;
      err = 0;
    }
    else {
      throw std::system_error(ENOSYS, std::generic_category(), "SSH_Sanitizer");
    }
  } catch(const std::system_error& error) {
      std::cout << "ERROR: " << error.code() << " - " << error.what() << std::endl;
      err = error.code().value();
      builder_cleanup();
  } catch(const boost::filesystem::filesystem_error& error) {
      std::cout << "ERROR: " << error.what() << std::endl;
      err = EXIT_FAILURE;
      builder_cleanup();
  }

  return err;
}
