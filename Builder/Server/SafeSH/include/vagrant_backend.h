#pragma once

#include <string>
#include "build_queue.h"

namespace builder {
    class VagrantBackend {

    public:
        VagrantBackend() = default;

        ~VagrantBackend();

        VagrantBackend(const VagrantBackend &) = delete;

        VagrantBackend &operator=(const VagrantBackend &)  = delete;

        VagrantBackend(VagrantBackend &&) noexcept = delete;

        VagrantBackend &operator=(VagrantBackend &&)       = delete;

        int build();

    private:
        void bring_up();

        void tear_down();
    };
}