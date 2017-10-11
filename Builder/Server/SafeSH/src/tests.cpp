#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include <stdlib.h>
#include <functional>
#include "ssh_sanitizer.h"
#include "sqlite3.h"
#include <boost/filesystem.hpp>

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

TEST_CASE("SSH_Sanitizer() requires a single argument to be provided") {
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

TEST_CASE("SSH_Sanitizer() requires SSH_CONNECTION to be set correctly") {
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

TEST_CASE("SSH_Sanitizer.sanitized_run() should only allow whitelisted commands") {
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

class tmp_db {
  public:
    tmp_db() {
      sqlite3 *db = NULL;
      int rc;
      rc = sqlite3_open_v2("Builder.db", &db, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, NULL);
      rc |= sqlite3_exec(db,"CREATE TABLE active_builds(id INTEGER PRIMARY KEY, count INTEGER);", NULL, NULL, NULL);
      rc |= sqlite3_exec(db,"CREATE TABLE build_queue(build_id INTEGER PRIMARY KEY AUTOINCREMENT);", NULL, NULL, NULL);
      rc |= sqlite3_exec(db,"INSERT INTO active_builds(id, count) VALUES(1, 0);", NULL, NULL, NULL);
      rc |= sqlite3_close(db);

      if(rc != SQLITE_OK) {
        throw std::system_error(ECONNABORTED, std::generic_category(), "Failed to create tmp_db for testing");
      }
    }
    ~tmp_db() {
      boost::filesystem::remove_all("./Builder.db");
    }
};
/*
TEST_CASE("SingularityBuilder() can be constructed if Builder.db SQLite db exists") {
  std::string work_path("/some/work/path");
  std::string build_id("1");

  SECTION("If Builder db exists SingularityBuilder() succeeds") {
    tmp_db db;
    REQUIRE_NOTHROW(builder::SingularityBuilder(work_path, build_id));
  }

  SECTION("If Builder db doesn't already exist SingularityBuilder() fails)") {
    REQUIRE_THROWS_WITH(builder::SingularityBuilder(work_path, build_id), Contains("Failed to init database"));
  }
}

TEST_CASE("The build queue works as expected") {
  tmp_db db;

   SECTION("the first job to enter the queue is at the top") {
     builder::SingularityBuilder job1("/work_path", "job1");
     job1.enter_queue();
     REQUIRE_THAT(job1.first_in_queue() == true);
   } 
}
*/
