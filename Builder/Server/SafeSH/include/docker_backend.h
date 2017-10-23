#pragma once

#include <string>
#include "build_queue.h"

namespace builder {
  class DockerBackend {

  public:
    DockerBackend() = default;

    ~DockerBackend();

    DockerBackend(const DockerBackend &) = delete;

    DockerBackend &operator=(const DockerBackend &)  = delete;

    DockerBackend(DockerBackend &&) noexcept = delete;

    DockerBackend &operator=(DockerBackend &&)       = delete;

    int build();

  private:
    void tear_down();

    std::string docker_name;
  };
}