@echo off
setlocal
set "PATH=%PATH%;C:\msys64\mingw64\bin"

cmake -B build -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE="%~dp0vcpkg\scripts\buildsystems\vcpkg.cmake" || exit /b 1
cmake --build build || exit /b 1
.\build\torus_example.exe || exit /b 1