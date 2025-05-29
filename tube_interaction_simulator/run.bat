@echo off
setlocal
set "PATH=%PATH%;C:\msys64\mingw64\bin"
set "ROOT=%~dp0"

set "BUILD_DIR=build_tube_interaction_simulator"
set "PROD_FLAG="

if "%1"=="prod" (
    set "BUILD_DIR=build_tube_interaction_simulator_prod"
    set "PROD_FLAG=-Dprod=ON"
)

rmdir /s /q %BUILD_DIR% 2>nul
mkdir %BUILD_DIR%

cmake -B %BUILD_DIR% -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE="%ROOT%\..\vcpkg\scripts\buildsystems\vcpkg.cmake" %PROD_FLAG%
cmake --build %BUILD_DIR% || exit /b 1
.\%BUILD_DIR%\tube_interaction_simulator.exe || exit /b 1