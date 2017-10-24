#include "vagrant_backend.h"
#include <iostream>
#include <string>
#include <system_error>
#include <cerrno>
#include "sql_db.h"
#include "signal_handler.h"
#include <boost/process.hpp>
#include "build_queue.h"

namespace builder {
#ifndef ENONET
#define ENONET 2
#endif
    namespace bp = boost::process;

    VagrantBackend::~VagrantBackend() {
        this->tear_down();
    }

    // Stop the vagrant VM and remove it
    void VagrantBackend::tear_down() {
        std::cerr << "Attempting to destroy VM..." << std::endl;
        std::string stop_command("vagrant destroy");
        std::error_code err;
        boost::process::system(stop_command, err);
        if (err.value() != 0) {
            std::cerr << "Failed to stop Vagrant VM!" << std::endl;
        }
    }

    // Wait for an open build spot and then Fire up the VM
    void VagrantBackend::bring_up() {
        std::string vagrant_up_command;
        vagrant_up_command += "vagrant up";

        // Launch the command asynchronously
        bp::child vagrant_proc(vagrant_up_command);

        // Test if we should stop vagrant
        while (vagrant_proc.running()) {
            if (gShouldKill) {
                this->tear_down();
            }
        }

        // Wait for vagrant to complete
        vagrant_proc.wait();
        int rc = vagrant_proc.exit_code();
        if (rc != 0) {
            this->tear_down();
            throw std::system_error(ENONET, std::generic_category(), "Vagrant bring_up failed!");
        }
    }

    // Run singularity build within vagrant VM
    int VagrantBackend::build() {
        BuildQueue queue;

        int rc = queue.run([&](std::string slot_id) {
            this->bring_up();

            std::string vagrant_build_command(
                    "vagrant ssh -c 'sudo singularity build /vagrant/container.img /vagrant/container.def'");
            bp::child vagrant_proc_build(vagrant_build_command);
            while (vagrant_proc_build.running()) {
                if (gShouldKill) {
                    this->tear_down();
                }
            }
            vagrant_proc_build.wait();

            this->tear_down();

            return vagrant_proc_build.exit_code();
        });

        return rc;
    }
}
