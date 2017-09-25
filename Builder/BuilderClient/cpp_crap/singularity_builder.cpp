#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>
#include <cstdio>
#include <signal.h>

// Build server base work directory
static constexpr auto work_base_path = "/home/builder/container_scratch/";

// Check that the IP of the build server is defined and not empty
static_assert(sizeof(BUILDER_IP) > 1, "BUILDER_IP must be defined!\n");
// Check that the SSH key file path is not empty
static_assert(sizeof(KEY_FILE) > 1, "KEY_FILE must be defined!\n");

namespace bp = boost::process;

// Handle remote communication with build server through SSH/SCP
class Builder() {
  public:
    Builder(const std::string, const std::string): ip{builder_ip}, key_path{key_path} {
      // Initiate an SSH control master connection
      this->control_pid = async_execv("/usr/bin/ssh -N -oControlMaster=yes -S%s -F /dev/null -i%s, -oStrictHostKeyChecking=no builder@%s",
                                       this->control_socket.c_str(), this->key_path.c_str(), this->ip.c_str());
     
      // Collect SSH_CONNECTION as defined on builder
      std::string ssh_connection_info = 
      work_directory += ssh_connection_info;
    };
    
    // Run a command over SSH on the builder
    int run(const char *command) {
      int err = blocking_execv("/usr/bin/ssh -S%s -F/dev/null -i%s '-oStrictHostKeyChecking=no' builder@%s %s",
                                this->control_socket.c_str(), this->key_path.c_str(), this->ip, command.c_str());
      return err;
    }

    // Copy file to builder using SCP command
    // SCP may only copy to /home/builder/container_work/this->work_dir
    int copy_to(const char* source) {
      int err = blocking_execv("scp -S%s -F/dev/null -i%s '-oStrictHostKeyChecking=no' builder@%s %s",
                                this->control_socket.c_str(), this->key_path.c_str(), this->ip, command.c_str());
      return err;
    }

    ~Builder() {
      // Kill SSH control master connection
      kill(control_pid, SIGKILL);
    }
  private:
    const std::string control_socket{"/tmp/.controlmaster-\%u-\%r@\%h:\%p"};
    pid_t control_pid;
    const std::string ip;
    const std::string key_path;
    std::string work_directory{"/home/builder/container_work"};
};

int main(int argc, char **argv) {
  // Test argument count
  if(argc != 4) {
    std::cerr<<"Usage: $ singularity_builder image_name path_to_definition size_in_megabytes\n";
    exit(EXIT_FAILURE);
  }

  // Extract arguments
  const auto container_name = argv[1];
  const auto container_def = argv[2];
  const auto container_size = argv[3];

  const std::vector<const char*> mkdir_call{"/bin/mkdir", work_dir.c_str()};
  SSH(mkdir_call);

  // Transfer definition file to build directory with name container.def
  std::string def_destination{"builder@" BUILDER_IP ":"};
  def_destination += work_dir;
  def_destination += "container.def";
  SCP(container_def, def_destination.c_str());  

  // Build container
  const std::vector<const char*> build_call{"/usr/local/bin/SingularityBuilder", container_size, UUID_c_string, work_dir.c_str()};
  SSH(build_call);

  // Transfer container back
  std::string source_container{"builder@" BUILDER_IP ":"};
  source_container += work_dir;
  source_container += "container.img";
  SCP(source_container.c_str(), container_name);

  return 0;
}
