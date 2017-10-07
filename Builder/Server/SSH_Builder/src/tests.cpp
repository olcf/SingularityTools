#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include <stdlib.h>
#include <functional>
#include "ssh_sanitizer.h"

using Catch::Contains;
using Catch::Equals;

// Fun the lambda func and capture any stdout output in the provided string
static void capture_stdout(std::function<void()>func, std::string& std_out) {
  std::stringstream buffer;
  std::streambuf * old_buffer = std::cout.rdbuf(buffer.rdbuf());
  func();
  std_out = buffer.str();
  std::cout.rdbuf(old_buffer);
}

TEST_CASE("SSH_Sanitizer(argc, argv) will not throw if SSH_CONNECTION is defined and a single argument is provided") {
  setenv("SSH_CONNECTION", "1.2.3.4 5678 9.10.11.12 131415", 1);
  int new_argc = 2;
  const char *cnew_argv[] = {"SSH_Sanitizer", "FooBar"};
  char ** new_argv = (char**)cnew_argv;
  REQUIRE_NOTHROW(builder::SSH_Sanitizer(new_argc, new_argv));      
}

TEST_CASE("SSH_Sanitizer(argc, argv) will throw if SSH_CONNECTION is not set and 1 argument is passed in") {
  unsetenv("SSH_CONNECTION");
  int new_argc = 2; 
  const char *cnew_argv[] = {"SSH_Sanitizer", "FooBar"};
  char ** new_argv = (char**)cnew_argv;
  REQUIRE_THROWS_WITH(builder::SSH_Sanitizer(new_argc, new_argv), Contains("SSH_CONNECTION"));
}

TEST_CASE("SSH_Sanitizer(argc, argv) will throw if SSH_CONNECTION is set and 0 arguments are passed in") {
  setenv("SSH_CONNECTION", "1.2.3.4 5678 9.10.11.12 131415", 1);
  int new_argc = 1;
  const char *cnew_argv[] = {"SSH_Sanitizer"};
  char ** new_argv = (char**)cnew_argv;
  REQUIRE_THROWS_WITH(builder::SSH_Sanitizer(new_argc, new_argv), Contains("argument"));
}
  
TEST_CASE("SSH_Sanitizer(argc, argv) will throw if SSH_CONNECTION is set and 2 arguments are passed in") {
  setenv("SSH_CONNECTION", "1.2.3.4 5678 9.10.11.12 131415", 1);
  int new_argc = 3;
  const char *cnew_argv[] = {"SSH_Sanitizer", "Foo", "Bar"};
  char ** new_argv = (char**)cnew_argv;
  REQUIRE_THROWS_WITH(builder::SSH_Sanitizer(new_argc, new_argv), Contains("argument"));
}

TEST_CASE("SSH_Sanitizer(argc, argv) will throw if SSH_CONNECTION is incorrectly set and 1 arguments is passed in") {
  setenv("SSH_CONNECTION", "1.2.3.4 5678 9.10.11.12", 1);
  int new_argc = 2;
  const char *cnew_argv[] = {"SSH_Sanitizer", "FooBar"};
  char ** new_argv = (char**)cnew_argv;
  REQUIRE_THROWS_WITH(builder::SSH_Sanitizer(new_argc, new_argv), Contains("SSH_CONNECTION"));
}

TEST_CASE("SSH_Sanitizer Should allow scp -t foo") {
  setenv("SSH_CONNECTION", "1.2.3.4 5678 9.10.11.12 131415", 1);
  int new_argc = 2;
  const char *cnew_argv[] = {"SSH_Sanitizer", "scp -t foo"};
  char ** new_argv = (char**)cnew_argv;
  builder::SSH_Sanitizer ssh_builder(new_argc, new_argv);

  std::string std_out;
  REQUIRE_NOTHROW(capture_stdout([&]() { ssh_builder.sanitized_run(); }, std_out));
  REQUIRE_THAT(std_out, Equals("scp -t /home/builder/container_scratch/1.2.3.4_5678_9.10.11.12_131415/container.def"));

}

TEST_CASE("SSH_Sanitizer Should allow scp -f bar") {
  setenv("SSH_CONNECTION", "1.2.3.4 5678 9.10.11.12 131415", 1);
  int new_argc = 2;
  const char *cnew_argv[] = {"SSH_Sanitizer", "scp -f bar"};
  char ** new_argv = (char**)cnew_argv;
  builder::SSH_Sanitizer ssh_builder(new_argc, new_argv);

  std::string std_out;
  REQUIRE_NOTHROW(capture_stdout([&]() { ssh_builder.sanitized_run(); }, std_out));
  REQUIRE_THAT(std_out, Equals("scp -f /home/builder/container_scratch/1.2.3.4_5678_9.10.11.12_131415/container.img"));
}

TEST_CASE("SSH_Sanitizer Should allow BuilderPrep") {
  setenv("SSH_CONNECTION", "1.2.3.4 5678 9.10.11.12 131415", 1);
  int new_argc = 2;
  const char *cnew_argv[] = {"SSH_Sanitizer", "BuilderPrep"};
  char ** new_argv = (char**)cnew_argv;
  builder::SSH_Sanitizer ssh_builder(new_argc, new_argv);
  REQUIRE_NOTHROW(ssh_builder.sanitized_run());

  std::string std_out;
  REQUIRE_NOTHROW(capture_stdout([&]() { ssh_builder.sanitized_run(); }, std_out));
  REQUIRE_THAT(std_out, Equals("mkdir /home/builder/container_scratch/1.2.3.4_5678_9.10.11.12_131415"));
}

TEST_CASE("SSH_Sanitizer Should allow BuilderRun") {
  setenv("SSH_CONNECTION", "1.2.3.4 5678 9.10.11.12 131415", 1);
  int new_argc = 2;
  const char *cnew_argv[] = {"SSH_Sanitizer", "BuilderRun"};
  char ** new_argv = (char**)cnew_argv;
  builder::SSH_Sanitizer ssh_builder(new_argc, new_argv);
  REQUIRE_NOTHROW(ssh_builder.sanitized_run());

  std::string std_out;
  REQUIRE_NOTHROW(capture_stdout([&]() { ssh_builder.sanitized_run(); }, std_out));
  REQUIRE_THAT(std_out, Equals("builder.build()"));
}

TEST_CASE("SSH_Sanitizer Should allow BuilderCleanup") {
  setenv("SSH_CONNECTION", "1.2.3.4 5678 9.10.11.12 131415", 1);
  int new_argc = 2;
  const char *cnew_argv[] = {"SSH_Sanitizer", "BuilderCleanup"};
  char ** new_argv = (char**)cnew_argv;
  builder::SSH_Sanitizer ssh_builder(new_argc, new_argv);
  REQUIRE_NOTHROW(ssh_builder.sanitized_run());

  std::string std_out;
  REQUIRE_NOTHROW(capture_stdout([&]() { ssh_builder.sanitized_run(); }, std_out));
  REQUIRE_THAT(std_out, Equals("rm /home/builder/container_scratch/1.2.3.4_5678_9.10.11.12_131415"));
}
