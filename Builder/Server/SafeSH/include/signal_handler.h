#pragma once

#include <csignal>

namespace builder {
  extern std::sig_atomic_t gShouldKill;

  void signal_handler(int signal);
}
