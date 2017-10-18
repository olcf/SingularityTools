#include "utilities.h"

#include <functional>
#include <iostream>
#include <string>
#include <sstream>
#include "sqlite3.h"
#include <boost/filesystem.hpp>

// Run the provided lambda func and capture any stdout output in the provided string
void capture_stdout(std::function<void()>func, std::string& std_out) {
  std::stringstream buffer;
  std::streambuf * old_buffer = std::cout.rdbuf(buffer.rdbuf());
  try {
    func();
  } catch(...) {
      std::cout<<buffer.str();
      std::cout.rdbuf(old_buffer);
      throw;
  }
  std_out = buffer.str();
  std::cout.rdbuf(old_buffer);
}

TMP_DB::TMP_DB() {

    boost::filesystem::remove_all("./BuildQueue.db");
    boost::filesystem::remove_all("./ResourceManager.db");

    sqlite3 *queue_db = nullptr;
    int rc;
    rc = sqlite3_open_v2("BuildQueue.db", &queue_db, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, nullptr);
    rc |= sqlite3_exec(queue_db, "CREATE TABLE queue(id INTEGER PRIMARY KEY, status TEXT, timestamp INTEGER DEFAULT (strftime('%s','now')));",
                       nullptr,
                       nullptr,
                       nullptr);
    rc |= sqlite3_close(queue_db);
    if (rc != SQLITE_OK) {
        throw std::system_error(ECONNABORTED, std::generic_category(), "Failed to create tmp build DB for testing");
    }

    sqlite3 *resource_db = nullptr;
    rc = sqlite3_open_v2("ResourceManager.db", &resource_db, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, nullptr);
    rc |= sqlite3_exec(resource_db, "CREATE TABLE slot(id INTEGER PRIMARY KEY, status TEXT);", nullptr, nullptr, nullptr);
    rc |= sqlite3_exec(resource_db, "INSERT INTO slot(status) VALUES('f');", nullptr, nullptr, nullptr);
    rc |= sqlite3_exec(resource_db, "INSERT INTO slot(status) VALUES('f');", nullptr, nullptr, nullptr);
    rc |= sqlite3_close(resource_db);

    if (rc != SQLITE_OK) {
        throw std::system_error(ECONNABORTED, std::generic_category(), "Failed to create tmp resource DB for testing");
    }
}

TMP_DB::~TMP_DB() {
    boost::filesystem::remove_all("./BuildQueue.db");
    boost::filesystem::remove_all("./ResourceManager.db");
}
