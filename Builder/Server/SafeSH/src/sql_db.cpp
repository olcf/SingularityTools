#include "sql_db.h"
#include "sqlite3.h"
#include <iostream>
#include <system_error>

namespace builder {

  static sqlite3* db_init(std::string db_file) {
    sqlite3* db = NULL;
    int db_rc = sqlite3_open_v2(db_file.c_str(), &db, SQLITE_OPEN_READWRITE, NULL);
    if(db_rc != SQLITE_OK) {
      sqlite3_close(db);
      throw std::system_error(ECONNABORTED, std::generic_category(), "Failed to init database");
    }
    return db;
  }

  SQL::SQL(std::string db_file) : db_file{db_file},
                                  db{db_init(db_file)}
  {}

  SQL::~SQL() {
    int rc = sqlite3_close(this->db);
    if(rc != SQLITE_OK)
      std::cerr<<"Error closing database!\n";
  }

 void SQL::exec(std::string sql_command, int (*callback)(void*,int,char**,char**), void *callback_arg, bool should_throw) {
    char *db_err = NULL;

    int rc = sqlite3_exec(this->db, sql_command.c_str(), callback, callback_arg, &db_err);
    if(rc != SQLITE_OK) {
      std::string err(db_err);
      sqlite3_free(db_err);
      if(should_throw) {
          std::string throw_err("Error: ");
          throw_err += err + " in command: " + sql_command + " ";
          throw std::system_error(ECONNABORTED, std::generic_category(), throw_err);
      }
      else {
          std::cerr << "Error: " << err << std::endl;
      }
    }
 }

  sqlite3_int64 SQL::last_insert_rowid() {
    return sqlite3_last_insert_rowid(this->db);
  }

  int SQL::changes() {
    return sqlite3_changes(this->db);
  };

}
