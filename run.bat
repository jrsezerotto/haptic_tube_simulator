set "PATH=%PATH%;C:\msys64\mingw64\bin"
cmake -B build -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE="C:/Users/Junior/Desktop/mestrado/omega6/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build
.\build\torus_example.exe