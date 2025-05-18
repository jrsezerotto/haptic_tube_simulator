@echo off
setlocal
set "PATH=%PATH%;C:\msys64\mingw64\bin"

cmake -S services/haptic_processor -B build_processor -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE="%~dp0vcpkg\scripts\buildsystems\vcpkg.cmake" || exit /b 1
cmake --build build_processor || exit /b 1
.\build_processor\haptic_processor.exe || exit /b 1