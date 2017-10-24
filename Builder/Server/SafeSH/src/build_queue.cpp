#include "build_queue.h"
#include <iostream>
#include <string>
#include <system_error>
#include "sql_db.h"
#include <chrono>
#include <thread>

namespace builder {
#ifdef DEBUG
    static constexpr auto build_database = "./BuildQueue.db";
#else
    static constexpr auto build_database = "/home/builder/BuildQueue.db";
#endif

    // TODO: pass the function to run into constructor//

    // Upon construction the build will be added to the queue
    BuildQueue::BuildQueue() : db{build_database},
                               build_id{this->enter()} {
    }

    BuildQueue::~BuildQueue() {
        this->exit(NO_THROW);
    }

    // Enter a new build into the queue and return it's build id
    std::string BuildQueue::enter() {
        std::string insert_command = std::string() + "INSERT INTO queue (status) VALUES (\"" +
                                     static_cast<char>(JobStatus::queued) + "\");";
        this->db.exec(insert_command, nullptr, nullptr);

        // rowid is an alias for the primary key
        return std::to_string(db.last_insert_rowid());
    }

    // Remove a build from the queue
    void BuildQueue::exit(bool should_throw) {
        this->set_status(JobStatus::finished, should_throw);
    }

    void BuildQueue::set_status(JobStatus status, bool should_throw) {
        if (this->build_id.empty()) {
            std::cerr << "Error setting build queue status: build_id not set!\n";
            return;
        }
        std::string status_command = std::string() + "UPDATE queue SET status = \"" + static_cast<char>(status) +
                                     "\" WHERE id = \"" + this->build_id + "\";";
        db.exec(status_command, nullptr, nullptr, should_throw);
    }

    // Return true if the specified build job is at the top of the queue
    static int top_of_queue_callback(void *first_in_queue, int count, char **values, char **names) {
        *static_cast<std::string *>(first_in_queue) = values[0];
        return 0;
    }

    bool BuildQueue::top() {
        std::string first_in_queue;
        std::string select_command = std::string() + "SELECT id FROM queue WHERE status = \"" +
                                     static_cast<char>(JobStatus::queued) +
                                     "\" ORDER BY timestamp ASC LIMIT 1;";
        db.exec(select_command, top_of_queue_callback, &first_in_queue);
        return first_in_queue == build_id;
    }
}