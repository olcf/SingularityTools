project(SSH_Builder)
cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")

add_executable(SSH_Builder src/main.cpp src/signal_handler.cpp src/ssh_sanitizer.cpp src/singularity_builder.cpp)
add_executable(SSH_BuilderTests src/tests.cpp src/signal_handler.cpp src/ssh_sanitizer.cpp src/singularity_builder.cpp)

include_directories(${CMAKE_SOURCE_DIR}/include)

# Setup Tests
set_target_properties(SSH_BuilderTests
                      PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS} -DDEBUG=1 -fsanitize=undefined -fno-omit-frame-pointer -fsanitize=address")
set_target_properties(SSH_BuilderTests
                      PROPERTIES LINK_FLAGS "${LINK_FLAGS} -fsanitize=undefined -fno-omit-frame-pointer -fsanitize=address")

# Includes Catch
# The catch project will fetch the headers from github at build time
add_subdirectory(CMake/catch)
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})
include_directories(${CATCH_INCLUDE_DIR})
add_dependencies(SSH_BuilderTests catch)
add_test(NAME SSH_BuilderTests COMMAND SSH_SanitizerTests)
target_link_libraries(SSH_BuilderTests Catch)

# Include Boost
# CMAKE doesn't seem to support the process component yet...
find_package(Boost 1.64.0 COMPONENTS system filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(SSH_Builder ${Boost_LIBRARIES})
target_link_libraries(SSH_BuilderTests ${Boost_LIBRARIES})

# Add SQLite
find_package(PkgConfig)
pkg_search_module(SQLITE3 REQUIRED sqlite3)
include_directories(${SQLITE3_INCLUDE_DIRS})
target_link_libraries(SSH_Builder ${SQLITE3_LIBRARIES})
target_link_libraries(SSH_BuilderTests ${SQLITE3_LIBRARIES})

install(TARGETS SSH_Builder DESTINATION bin)
install(TARGETS SSH_BuilderTests DESTINATION bin)
