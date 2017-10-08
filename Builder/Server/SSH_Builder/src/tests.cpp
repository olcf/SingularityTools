#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include <stdlib.h>
#include <functional>
#include "ssh_sanitizer.h"

using Catch::Contains;
using Catch::Equals;

// Run the provided lambda func and capture any stdout output in the provided string
static void capture_stdout(std::function<void()>func, std::string& std_out) {
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

TEST_CASE("SSH_Sanitizer will only suceed if a single argument is provided") {
  setenv("SSH_CONNECTION", "1.2.3.4 5678 9.10.11.12 131415", 1);

  SECTION("Providing exactly one argument succeeds") {
    int new_argc = 2;
    const char *cnew_argv[] = {"SSH_Sanitizer", "FooBar"};
    char ** new_argv = (char**)cnew_argv;
    REQUIRE_NOTHROW(builder::SSH_Sanitizer(new_argc, new_argv));      
  }

  SECTION("Providing zero arguments fails") {
    int new_argc = 1;
    const char *cnew_argv[] = {"SSH_Sanitizer"};
    char ** new_argv = (char**)cnew_argv;
    REQUIRE_THROWS_WITH(builder::SSH_Sanitizer(new_argc, new_argv), Contains("argument"));
  }
  
  SECTION("Providing 2 arguments fails") {
    int new_argc = 3;
    const char *cnew_argv[] = {"SSH_Sanitizer", "Foo", "Bar"};
    char ** new_argv = (char**)cnew_argv;
    REQUIRE_THROWS_WITH(builder::SSH_Sanitizer(new_argc, new_argv), Contains("argument"));
  }
}

TEST_CASE("SSH_Sanitizer will throw if SSH_CONNECTION is not set correctly") {
  SECTION("an unset SSH_CONNECTION with valid arguments will fail") {
    unsetenv("SSH_CONNECTION");
    int new_argc = 2;
    const char *cnew_argv[] = {"SSH_Sanitizer", "FooBar"};
    char ** new_argv = (char**)cnew_argv;
    REQUIRE_THROWS_WITH(builder::SSH_Sanitizer(new_argc, new_argv), Contains("SSH_CONNECTION"));
  }

  SECTION("SSH_CONNECTION will fail if formated incorrectly") {
    setenv("SSH_CONNECTION", "1.2.3.4 5678 9.10.11.12", 1);
    int new_argc = 2;
    const char *cnew_argv[] = {"SSH_Sanitizer", "FooBar"};
    char ** new_argv = (char**)cnew_argv;
    REQUIRE_THROWS_WITH(builder::SSH_Sanitizer(new_argc, new_argv), Contains("SSH_CONNECTION"));
  }
}

TEST_CASE("SSH_Sanitizer should only allow whitelisted commands") {
  setenv("SSH_CONNECTION", "1.2.3.4 5678 9.10.11.12 131415", 1);

  SECTION("scp -t container.def should be allowed") {
    int new_argc = 2;
    const char *cnew_argv[] = {"SSH_Sanitizer", "scp -t container.def"};
    char ** new_argv = (char**)cnew_argv;
    builder::SSH_Sanitizer ssh_builder(new_argc, new_argv);

    std::string std_out;
    REQUIRE_NOTHROW(capture_stdout([&]() { ssh_builder.sanitized_run(); }, std_out));
    REQUIRE_THAT(std_out, Equals("scp -t /home/builder/container_scratch/1.2.3.4_5678_9.10.11.12_131415/container.def"));
  }

  SECTION("SSH_Sanitizer should allow scp -f container.img") {
    int new_argc = 2;
    const char *cnew_argv[] = {"SSH_Sanitizer", "scp -f container.img"};
    char ** new_argv = (char**)cnew_argv;
    builder::SSH_Sanitizer ssh_builder(new_argc, new_argv);

    std::string std_out;
    REQUIRE_NOTHROW(capture_stdout([&]() { ssh_builder.sanitized_run(); }, std_out));
    REQUIRE_THAT(std_out, Equals("scp -f /home/builder/container_scratch/1.2.3.4_5678_9.10.11.12_131415/container.img"));
  }

  SECTION("SSH_Sanitizer should allow BuilderPrep") {
    int new_argc = 2;
    const char *cnew_argv[] = {"SSH_Sanitizer", "BuilderPrep"};
    char ** new_argv = (char**)cnew_argv;
    builder::SSH_Sanitizer ssh_builder(new_argc, new_argv);

    std::string std_out;
    REQUIRE_NOTHROW(capture_stdout([&]() { ssh_builder.sanitized_run(); }, std_out));
    REQUIRE_THAT(std_out, Equals("mkdir /home/builder/container_scratch/1.2.3.4_5678_9.10.11.12_131415"));
  }

  SECTION("SSH_Sanitizer should allow BuilderRun") {
    int new_argc = 2;
    const char *cnew_argv[] = {"SSH_Sanitizer", "BuilderRun"};
    char ** new_argv = (char**)cnew_argv;
    builder::SSH_Sanitizer ssh_builder(new_argc, new_argv);

    std::string std_out;
    REQUIRE_NOTHROW(capture_stdout([&]() { ssh_builder.sanitized_run(); }, std_out));
    REQUIRE_THAT(std_out, Equals("builder.build()"));
  }

  SECTION("SSH_Sanitizer should allow BuilderCleanup") {
    int new_argc = 2;
    const char *cnew_argv[] = {"SSH_Sanitizer", "BuilderCleanup"};
    char ** new_argv = (char**)cnew_argv;
    builder::SSH_Sanitizer ssh_builder(new_argc, new_argv);

    std::string std_out;
    REQUIRE_NOTHROW(capture_stdout([&]() { ssh_builder.sanitized_run(); }, std_out));
    REQUIRE_THAT(std_out, Equals("rm /home/builder/container_scratch/1.2.3.4_5678_9.10.11.12_131415"));
  }


  SECTION("SSH_Sanitizer should not allow BuilderCleanup;rm -rf /") {
    int new_argc = 2;
    const char *cnew_argv[] = {"SSH_Sanitizer", "BuilderCleanup;rm -rf /"};
    char ** new_argv = (char**)cnew_argv;

    REQUIRE_THROWS_WITH(builder::SSH_Sanitizer(new_argc, new_argv), Contains("invalid") && Contains("characters"));
  }

  SECTION("SSH_Sanitizer should not allow scp -f container && rm -rf /") {
    setenv("SSH_CONNECTION", "1.2.3.4 5678 9.10.11.12 131415", 1);
    int new_argc = 2;
    const char *cnew_argv[] = {"SSH_Sanitizer", "BuilderCleanup;rm -rf /"};
    char ** new_argv = (char**)cnew_argv;

    REQUIRE_THROWS_WITH(builder::SSH_Sanitizer(new_argc, new_argv), Contains("invalid") && Contains("characters"));
  }
}
