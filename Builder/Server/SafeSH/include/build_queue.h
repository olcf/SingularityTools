#pragma once
  
#include <string>
#include "sql_db.h"
#include "resource_manager.h"
#include "build_queue.h"
#include <functional>

namespace builder {
  class BuildQueue {

    enum class JobStatus: char {
      queued   = 'q',
      running  = 'r',
      finished = 'f',
      killed   = 'k'
    };

  public:
    // Constructors
    BuildQueue();
    ~BuildQueue();
    BuildQueue(const BuildQueue&)            = delete;
    BuildQueue& operator=(const BuildQueue&) = delete;
    BuildQueue(BuildQueue&&) noexcept        = delete;
    BuildQueue& operator=(BuildQueue&&)      = delete;

    // Run the specified function when queue allows
    int run(std::function<int()> func);
  private:
    SQL db;
    const std::string build_id;
    std::string enter();
    void exit(bool should_throw=true);
    bool top();
    void set_status(JobStatus status, bool should_throw=true);
  };
}