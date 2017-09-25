# Don't use Cray compile wrappers
find_program(CMAKE_C_COMPILER gcc)
find_program(CMAKE_CXX_COMPILER g++)

project(BuilderClient)
cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions(-DDEBUG=1)
endif()

if(NOT BUILDER_IP)
  message(FATAL_ERROR "BUILDER_IP must be set!")
endif()
if(NOT KEY_FILE)
  message(FATAL_ERROR "KEY_FILE must be set!")
endif()

add_definitions("-DBUILDER_IP=\"${BUILDER_IP}\"")
add_definitions("-DKEY_FILE=\"${KEY_FILE}\"")

add_executable(singularity_builder singularity_builder.cpp)

find_package(Boost COMPONENTS system filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(SSH_Sanitizer ${Boost_LIBRARIES})

find_package(PkgConfig REQUIRED)
pkg_search_module(UUID REQUIRED uuid)
target_link_libraries(singularity_builder ${UUID_LIBRARIES})

set(BUILDER_PERMISSIONS
    OWNER_WRITE OWNER_READ OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_EXECUTE)

install(TARGETS singularity_builder DESTINATION bin PERMISSIONS ${BUILDER_PERMISSIONS})
