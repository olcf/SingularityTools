#pragma once
  
#include <string>
#include "build_queue.h"

namespace builder {
  class VagrantBuilder {

    public:
      VagrantBuilder()                                  = default;
      ~VagrantBuilder();
      VagrantBuilder(const VagrantBuilder&)             = delete;
      VagrantBuilder& operator=(const VagrantBuilder&)  = delete;
      VagrantBuilder(VagrantBuilder&&) noexcept         = delete;
      VagrantBuilder& operator=(VagrantBuilder&&)       = delete;

      int build();
    private:
      void bring_up();
      void tear_down();
  };
}
