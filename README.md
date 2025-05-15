<!-- TODO(now): remove -->
> Libraries to install:
sudo apt-get install libusb-1.0-0-dev
sudo apt-get install libglu1-mesa-dev
sudo apt-get install libglfw3-dev

> To build on Windows:
$env:Path += ";C:\msys64\mingw64\bin"
cmake -B build -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE="C:/Users/Junior/Desktop/mestrado/omega6/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build