#pragma once

#include <functional>
#include <iostream>

class TMP_DB {
  public:
    TMP_DB();
    ~TMP_DB();
    TMP_DB(const TMP_DB&)               = delete;
    TMP_DB& operator=(const TMP_DB&)    = delete;
    TMP_DB(TMP_DB&&) noexcept           = delete;
    TMP_DB& operator=(TMP_DB&&)         = delete;

};

void capture_stdout(std::function<void()>func, std::string& std_out);
void capture_stdout(std::function<void()>func, std::string& std_out, std::streambuf* old_buffer);