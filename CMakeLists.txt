cmake_minimum_required(VERSION 3.0.0)
project(RobotControl VERSION 0.1.0)

include(CTest)
enable_testing()

# add_subdirectory(./external/chai3d-3.2.0/src/)
add_library(chai3d ./external/chai3d-3.2.0/lib/release/lin-x86_64-cc/libchai3d.a)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
