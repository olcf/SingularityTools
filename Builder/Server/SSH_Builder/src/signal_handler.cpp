#include <csignal>
#include "signal_handler.h"

namespace builder {
  std::sig_atomic_t gShouldKill = 0;

  void signal_handler(int signal) {
    gShouldKill = 1;
  };
}
