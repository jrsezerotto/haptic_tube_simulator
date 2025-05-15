cmake_minimum_required(VERSION 3.10)
project(example)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(example
   # sdk-3.17.6/examples/GLFW/sphere/sphere.cpp
   # sdk-3.17.6/examples/GLFW/torus/torus.cpp
   torus.cpp
   dhdc.cpp
)

include_directories(
   ${X11_INCLUDE_DIR}
   ${CMAKE_SOURCE_DIR}/sdk/include
   ${CMAKE_SOURCE_DIR}/sdk/examples/GLFW/sphere
   ${CMAKE_SOURCE_DIR}/sdk/externals/Eigen
)

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(X11 REQUIRED)

target_link_libraries(example PRIVATE 
   glfw
   OpenGL::GL
   GLU
   ${X11_LIBRARIES}
)
