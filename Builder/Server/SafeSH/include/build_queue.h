#pragma once

#include <string>
#include "sql_db.h"
#include "resource_manager.h"
#include "build_queue.h"
#include <functional>
#include <iostream>
#include "signal_handler.h"
#include <thread>
#include <system_error>

namespace builder {
    class BuildQueue {

    public:
        enum class JobStatus : char {
            queued = 'q',
            running = 'r',
            finished = 'f',
            killed = 'k'
        };

        // Constructors
        BuildQueue();

        ~BuildQueue();

        BuildQueue(const BuildQueue &) = delete;

        BuildQueue &operator=(const BuildQueue &) = delete;

        BuildQueue(BuildQueue &&) noexcept = delete;

        BuildQueue &operator=(BuildQueue &&)      = delete;

        static void print_spinner() {
#ifndef DEBUG
            std::cout << "Waiting for resources: .  \r" << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(600));
            std::cout << "Waiting for resources: .. \r" << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(600));
            std::cout << "Waiting for resources: ...\r" << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(600));
#endif
        }

        // Run the specified function when the job has made it's way to the top of the queue
        template<class F>
        auto run(F &&func) {
            ResourceManager build_resource;

            // Print an animation while waiting for available slot to open up
            while (!(this->top() && build_resource.reserve_slot())) {
                print_spinner();
                if (gShouldKill) {
                    this->set_status(JobStatus::killed);
                    std::cerr << "function killed in queue!\n";
                    throw std::system_error(EINTR, std::generic_category(), "queue run() interrupted by signal");
                }
            }

            // Run the specified function when slot is available
            this->set_status(JobStatus::running);
            auto return_value = func(build_resource.slot_id);
            this->set_status(JobStatus::finished);
            return return_value;
        };

        // We use shared_ptr instead of reference so that it's cleaned up at program exit
        static SQL& db();

        static int get_count(JobStatus status);

    private:
        const std::string build_id;

        std::string enter();

        void exit(bool should_throw = true);

        bool top();

        void set_status(JobStatus status, bool should_throw = true);
    };
}