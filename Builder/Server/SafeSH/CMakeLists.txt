project(SafeSH)
cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE -fpie")# -Wl,-z,noexecstack,-z,now,-z,relro,-z,nodlopen")
set(CMAKE_VERBOSE_MAKEFILE ON)

set(SRCS src/build_queue.cpp
         src/resource_manager.cpp
         src/signal_handler.cpp
         src/sql_db.cpp
         src/ssh_sanitizer.cpp
         src/vagrant_builder.cpp)

add_executable(SafeSH src/main.cpp ${SRCS})
add_executable(SafeSH_Tests src/tests.cpp ${SRCS})

include_directories(${CMAKE_SOURCE_DIR}/include)

# Setup Tests
set_target_properties(SafeSH_Tests
                      PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} -DDEBUG=1 -fsanitize=address,undefined -fno-omit-frame-pointer")
set_target_properties(SafeSH_Tests
                      PROPERTIES LINK_FLAGS "${LINK_FLAGS} -fsanitize=address,undefined -fno-omit-frame-pointer")

# Includes Catch
# The catch project will fetch the headers from github at build time
add_subdirectory(CMake/catch)
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})
include_directories(${CATCH_INCLUDE_DIR})
add_dependencies(SafeSH_Tests catch)
add_test(NAME SafeSH_Tests COMMAND SafeSH_Tests)
target_link_libraries(SafeSH_Tests Catch)

# Include Boost
# CMAKE doesn't seem to support the process component yet...
find_package(Boost 1.64.0 COMPONENTS system filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(SafeSH ${Boost_LIBRARIES})
target_link_libraries(SafeSH_Tests ${Boost_LIBRARIES})

# Add SQLite
find_package(PkgConfig)
pkg_search_module(SQLITE3 REQUIRED sqlite3)
include_directories(${SQLITE3_INCLUDE_DIRS})
target_link_libraries(SafeSH ${SQLITE3_LIBRARIES})
target_link_libraries(SafeSH_Tests ${SQLITE3_LIBRARIES})

install(TARGETS SafeSH DESTINATION bin)
install(TARGETS SafeSH_Tests DESTINATION bin)
