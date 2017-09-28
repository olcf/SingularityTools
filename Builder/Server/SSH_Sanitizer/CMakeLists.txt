project(SSH_Sanitizer)
cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions(-DDEBUG=1)
endif()

add_executable(SSH_Sanitizer SSH_sanitizer.cpp)

# CMAKE doesn't seem to support the process component yet...
find_package(Boost 1.64.0 COMPONENTS system filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(SSH_Sanitizer ${Boost_LIBRARIES})

install(TARGETS SSH_Sanitizer DESTINATION bin)
