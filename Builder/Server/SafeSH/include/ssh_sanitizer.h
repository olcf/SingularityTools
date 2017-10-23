#pragma once

#include <string>
#include <vector>

// Execute a sanitized copy of $SSH_ORIGINAL_COMMAND, as provided by the ssh authorized_keys command option
// A single string argument is expected so it must be quoted when setting command, e.g.
// command="/path/to/CommandSanitizer \"${SSH_ORIGINAL_COMMAND}\"" ssh-rsa ...
// the SSH_CONNECTION info is used as a unique identifier to ensure users only have very limited access

// The following commands are allowed:
// scp -t unique_work_path()/container.def
// scp -f unique_work_path()/container.name
// GetWorkPath
// BuilderPrep
// BuilderRun
// BuilderCleanup

// Note that SCP is a two way command and once it's finished on the initiating side it will return
// So we can't get fancy and have the SCP call do the prep/build/cleanup

namespace builder {
  class SSH_Sanitizer {

  public:
    // Constructors
    SSH_Sanitizer(int argc, char** argv);
    ~SSH_Sanitizer()                               = default;
    SSH_Sanitizer()                                = delete;
    SSH_Sanitizer(const SSH_Sanitizer&)            = delete;
    SSH_Sanitizer& operator=(const SSH_Sanitizer&) = delete;
    SSH_Sanitizer(SSH_Sanitizer&&) noexcept        = delete;
    SSH_Sanitizer& operator=(SSH_Sanitizer&&)      = delete;

    // Attempt to run the sanitized command
    int sanitized_run();

  private:
    int run_scp_to();
    int run_scp_from();
    int run_builder_prep();
    int run_builder();
    int run_builder_cleanup();
    int blocking_exec(std::string command);

    std::string unique_id;
    std::string unique_work_path;
    std::string command;
  };
}