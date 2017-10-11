#pragma once
  
#include <string>

namespace builder {
  class VagrantBuilder {

    public:
      // Constructors
      VagrantBuilder();
      ~VagrantBuilder();
      VagrantBuilder(const VagrantBuilder&)             = delete;
      VagrantBuilder& operator=(const VagrantBuilder&)  = delete;
      VagrantBuilder(VagrantBuilder&&) noexcept         = delete;
      VagrantBuilder& operator=(VagrantBuilder&&)       = delete;

      int build();
    private:
      bool active;
      void bring_up();
      void destroy();
  };
}
