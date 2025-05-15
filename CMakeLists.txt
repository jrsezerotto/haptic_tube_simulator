cmake_minimum_required(VERSION 3.14)

# Caminho do vcpkg (ajuste se necessário)
set(CMAKE_TOOLCHAIN_FILE "vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

project(torus_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(torus_example
    torus.cpp
    dhdc.cpp
)

# Includes
target_include_directories(torus_example PRIVATE
    ${CMAKE_SOURCE_DIR}/sdk/include
    ${CMAKE_SOURCE_DIR}/sdk/examples/GLFW/torus
    ${CMAKE_SOURCE_DIR}/sdk/externals/Eigen
)

# Bibliotecas
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

target_link_directories(torus_example PRIVATE
    ${CMAKE_SOURCE_DIR}/sdk/lib
)

target_link_libraries(torus_example PRIVATE
    glfw
    OpenGL::GL
    glu32
    dhdms64
)

# Subsystem console (somente se MSVC estiver sendo usado)
if(MSVC)
    set_target_properties(torus_example PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()