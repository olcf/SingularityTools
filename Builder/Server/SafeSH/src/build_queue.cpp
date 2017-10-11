#include "build_queue.h"
#include <iostream>
#include <string>
#include <system_error>
#include "sql_db.h"
#include <chrono>
#include <thread>
#include "resource_manager.h"

namespace builder {
  #ifdef DEBUG
    static constexpr auto gDatabase = "./BuildQueue.db";
  #else
    static constexpr auto gDatabase = "/home/builder/BuildQueue.db";
  #endif 

  // Enter a new build into the queue and return it's build id
  std::string enter(SQL& db) {
    std::string insert_command("INSERT INTO queue DEFAULT VALUES;");
    db.exec(insert_command, NULL, NULL);

    // rowid is an alias for our autoincrimenting primary key
    return std::to_string(db.last_insert_rowid());
  }

  // Upon construction the build will be added to the queue
  BuildQueue::BuildQueue() : db{gDatabase},
                             build_id{enter(db)} {
    this->queued = true;
  }

  BuildQueue::~BuildQueue() {
    if(this->queued)
      this->exit();
  }

  void BuildQueue::reserve_build_slot() {
    // Print an animation while waiting for available slot to open up
    while(!this->top() && !resources.reserve_build_slot()) {
      std::cout<<"Waiting for resources: .  \r" << std::flush;
      std::this_thread::sleep_for(std::chrono::milliseconds(600));
      std::cout<<"Waiting for resources: .. \r" << std::flush;
      std::this_thread::sleep_for(std::chrono::milliseconds(600));
      std::cout<<"Waiting for resources: ...\r" << std::flush;
      std::this_thread::sleep_for(std::chrono::milliseconds(600));

       //TODO test for interupt?
    }
      
    this->exit();   
  }

  // Remove a build from the queue
  void BuildQueue::exit() {
    std::string SQL_command;
    SQL_command += "DELETE FROM queue WHERE build_id = " + this->build_id + ";";
    db.exec(SQL_command, NULL, NULL, NO_THROW);
    this->queued = false;

    if(resources.build_slot_reserved)
      resources.release_build_slot();
  }

  // Return true if the specified build is at the top of the queue
  static int top_of_queue_callback(void *first_in_queue, int count, char** values, char** names) {
    *static_cast<std::string*>(first_in_queue) = values[0];
    return 0;
  }
  bool BuildQueue::top() {
    std::string first_in_queue;
    std::string select_command("SELECT build_id FROM queue ORDER BY build_id ASC LIMIT 1;");
    db.exec(select_command, top_of_queue_callback, &first_in_queue);
    return first_in_queue == build_id;
  }
}
