cmake_minimum_required(VERSION 3.14)

set(prod OFF CACHE BOOL "Build in production mode")
set(CMAKE_TOOLCHAIN_FILE "../../vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

project(haptic_processor LANGUAGES CXX)
message(STATUS "prod = ${prod}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(prod)
    message(STATUS "Building in production mode (dhdc.cpp excluído)")
    set(SOURCES haptic_processor.cpp)
else()
    message(STATUS "Building in development mode (dhdc.cpp incluído)")
    set(SOURCES haptic_processor.cpp ../../dhdc.cpp)
endif()

add_executable(haptic_processor ${SOURCES})

target_include_directories(haptic_processor PRIVATE
    ${CMAKE_SOURCE_DIR}/../../sdk/include
    ${CMAKE_SOURCE_DIR}/../../sdk/examples/GLFW/torus
    ${CMAKE_SOURCE_DIR}/../../sdk/externals/Eigen
)

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

target_link_directories(haptic_processor PRIVATE
    ${CMAKE_SOURCE_DIR}/../../sdk/lib
)

target_link_libraries(haptic_processor PRIVATE
    glfw
    OpenGL::GL
    glu32
    ws2_32
    dhdms64
)

if(MSVC)
    set_target_properties(haptic_processor PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()
