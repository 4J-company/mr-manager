cmake_minimum_required(VERSION 3.26)
project(
  mr-manager
  VERSION 1.0.2
  LANGUAGES CXX
)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(folly REQUIRED)

add_library(mr-manager INTERFACE include/mr-manager/manager.hpp)

target_compile_features(mr-manager INTERFACE cxx_std_23)
target_link_libraries(mr-manager INTERFACE folly::folly)
target_include_directories(mr-manager INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

install(TARGETS mr-manager
        EXPORT mr-managerTargets
        DESTINATION lib
)

install(DIRECTORY include/
        DESTINATION include
)

install(EXPORT mr-managerTargets
        FILE mr-managerConfig.cmake
        DESTINATION lib/cmake/mr-manager
)
