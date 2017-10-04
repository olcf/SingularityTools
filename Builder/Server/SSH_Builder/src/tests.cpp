#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

SCENARIO("SSH_Sanitizer only allows valid commands") {
  GIVEN("SSH_CONNECTION is set") {
    WHEN("some stuff happens") {
      THEN("bloop") {
      }
    }
  }
}
