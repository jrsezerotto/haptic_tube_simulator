cmake_minimum_required(VERSION 3.14)
set(prod OFF CACHE BOOL "Build in production mode")

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

project(tube_interaction_simulator LANGUAGES CXX)
message(STATUS "prod = ${prod}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(prod)
    message(STATUS "Building in production mode (dhdc.cpp excluído)")
    set(SOURCES tube_interaction_simulator.cpp)
    add_definitions(-DPROD_BUILD)
else()
    message(STATUS "Building in development mode (dhdc.cpp incluído)")
    set(SOURCES tube_interaction_simulator.cpp dhdc.cpp)
endif()

add_executable(tube_interaction_simulator ${SOURCES})

target_include_directories(tube_interaction_simulator PRIVATE
    ${CMAKE_SOURCE_DIR}/../sdk/include
    ${CMAKE_SOURCE_DIR}/../sdk/examples/GLFW/torus
    ${CMAKE_SOURCE_DIR}/../sdk/externals/Eigen
)

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

target_link_directories(tube_interaction_simulator PRIVATE
    ${CMAKE_SOURCE_DIR}/../sdk/lib
)

target_link_libraries(tube_interaction_simulator PRIVATE
    glfw
    OpenGL::GL
    glu32
    dhdms64
)