cmake_minimum_required(VERSION 3.0.0)
project(trx VERSION 0.1.0)
INCLUDE_DIRECTORIES(
	/home/valeriehayot/Documents/code/vcpkg/installed/x64-linux/include
)

LINK_DIRECTORIES(
	/home/valeriehayot/Documents/code/vcpkg/installed/x64-linux/lib
)
include(CTest)
enable_testing()

add_executable(trx src/trx.cpp)
TARGET_LINK_LIBRARIES(trx
	jsoncpp
	zip
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
