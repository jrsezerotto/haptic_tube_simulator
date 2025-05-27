cmake_minimum_required(VERSION 3.14)
set(prod OFF CACHE BOOL "Build in production mode")

# Caminho do vcpkg (ajuste se necessário)
set(CMAKE_TOOLCHAIN_FILE "vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

project(torus_example LANGUAGES CXX)
message(STATUS "prod = ${prod}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Se a flag -Dprod=ON for passada, dhdc.cpp é omitido
if(prod)
    message(STATUS "Building in production mode (dhdc.cpp excluído)")
    set(SOURCES sphere.cpp)
    target_compile_definitions(torus_example PRIVATE PROD_BUILD)
else()
    message(STATUS "Building in development mode (dhdc.cpp incluído)")
    set(SOURCES sphere.cpp dhdc.cpp)
endif()

add_executable(torus_example ${SOURCES})

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

if(MSVC)
    set_target_properties(torus_example PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()
