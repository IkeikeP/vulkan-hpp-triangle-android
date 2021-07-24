# Vulkan-hpp Triangle for Android
A minimalist implementation of the "hello triangle" in C++ using Vulkan-hpp. Supports Windows & Android. <br/>
For whoever is learning vulkan by following the [vulkan-tutorial](https://vulkan-tutorial.com/) and wants to quickly check the Vulkan-hpp equivalent code, this is for you. 
And with no surprises, most of the vulkan logic was based on that tutorial.

Highlights:
- C++ first (e.g. avoided Java where possible)
- Terminal friendly with no need to resort to IDEs like Visual Studio
- Modern and clean CMake
- Some inline comments explaining vulkan code
- Android support with clutter free gradle files

## Build

### Windows
```bash
mkdir build
cd build
cmake .. -G Ninja # add "-DCMAKE_BUILD_TYPE=Release" for release mode
ninja
```
Note: For MSVC, these commands need to be run within a [Developer Command Prompt](https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-160).

### Android
Open Android Studio, Sync Project and then press make.

On the terminal:
```bash
cd android
gradlew assembleDebug
```

## Dependencies
- [SDL 2](https://www.libsdl.org) (for Window management)

## Requirements
- [CMake 3.16.0](https://cmake.org/)
- [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/)
- C++ Compiler (e.g. MSVC or clang)
- [Ninja](https://ninja-build.org/) or other build system such as Visual Studio
- Android Studio / Android SDK and NDK