#pragma once
  
#include <string>
#include "sqlite3.h"

namespace builder {
  constexpr bool NO_THROW = false;

  class SQL {

  public:
    SQL(std::string db_file);
    ~SQL();
    SQL()                        = delete;
    SQL(const SQL&)              = delete;
    SQL& operator=(const SQL&)   = delete;
    SQL(SQL&&) noexcept          = delete;
    SQL& operator=(SQL&&)        = delete;

    void exec(std::string sql, int (*callback)(void*,int,char**,char**), void *callback_arg, bool should_throw=true);
    sqlite3_int64 last_insert_rowid();
  private:
    std::string db_file;
    const sqlite3 *db;
  };
}
