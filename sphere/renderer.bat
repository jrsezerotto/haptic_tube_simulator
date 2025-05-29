@echo off
setlocal
set "PATH=%PATH%;C:\msys64\mingw64\bin"
set "ROOT=%~dp0"

cmake -S services/haptic_renderer -B build_renderer -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE="%ROOT%\..\vcpkg\scripts\buildsystems\vcpkg.cmake" || exit /b 1
cmake --build build_renderer || exit /b 1
.\build_renderer\haptic_renderer.exe || exit /b 1