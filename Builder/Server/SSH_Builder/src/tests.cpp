#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include <stdlib.h>
#include "ssh_sanitizer.h"

SCENARIO("SSH_Sanitizer can be constructed") {
  GIVEN("SSH_CONNECTION is set and 1 argument is passed in") {
    setenv("SSH_CONNECTION", "1.2.3.4 5678 9.10.11.12 131415", 0);
    int new_argc = 2;
    const char *new_argv[] = {"SSH_Sanitizer", "FooBar"};
    char ** new_argv = (char**)cnew_argv;
    WHEN("SSH_Sanitizer is constructed") {
      THEN("No exception is thrown") {
        REQUIRE_NOTHROW(builder::SSH_Sanitizer(new_argc, new_argv));      
      }
    }
  }
}
