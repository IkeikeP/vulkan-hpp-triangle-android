# Project Info
cmake_minimum_required(VERSION 3.16)
cmake_policy(VERSION 3.16)
project(Naru VERSION 1.0.0 LANGUAGES CXX)

if (ANDROID)
    add_library(${PROJECT_NAME} SHARED)
else()
    add_executable(${PROJECT_NAME})
endif()
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

if (NOT ANDROID)
    # Build type
    if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release ... FORCE )
    endif()
    if(CMAKE_BUILD_TYPE MATCHES Debug)
        target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=1)
    endif()
    message("Build type: ${CMAKE_BUILD_TYPE}")

endif()
# Sources directory
add_subdirectory(src)

# SPIR-V Compilation
add_subdirectory(shaders)

# Only suitable if SOURCES does not contain generated files in this example
get_target_property(sources ${PROJECT_NAME} SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
             PREFIX "Naru\\Sources"
             FILES ${sources}
)

# Dependencies
# Vulkan

# This is required to allow us to compile the Vulkan C++ header successfully.
if(ANDROID)
    add_definitions(-DVULKAN_HPP_TYPESAFE_CONVERSION)
    set(VULKAN_DIR ${ANDROID_NDK}/sources/third_party/vulkan/src)
    set(ASSET_MANAGER ${ANDROID_NDK}/sysroot/usr/include)
    set(Vulkan_INCLUDE_DIR ${VULKAN_DIR}/include)
    target_include_directories(${PROJECT_NAME} PRIVATE ${VULKAN_DIR}/common ASSET_MANAGER)
    add_library(VKWRAPPER SHARED ${VULKAN_DIR}/common/vulkan_wrapper.cpp)
    target_link_libraries(${PROJECT_NAME} VKWRAPPER)
    target_include_directories(${PROJECT_NAME} PRIVATE android/app/src/main/cpp)

    # This is the built in NDK logging library
    find_library(log-lib log)
    target_link_libraries(${PROJECT_NAME} ${log-lib})
else()
    find_package(Vulkan REQUIRED)
    if(${VULKAN_FOUND})
        message("Vulkan_LIBRARY: " ${Vulkan_LIBRARY})
        message("Vulkan_INCLUDE_DIR: " ${Vulkan_INCLUDE_DIR}) 
        else() 
            message(FATAL_ERROR "Vulkan not found, please verify if Vulkan SDK is installed.")
        endif()
endif()

# SDL
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(HIDAPI OFF CACHE BOOL "" FORCE)
add_subdirectory(src/libs/sdl)

# Debug
if(CMAKE_BUILD_TYPE MATCHES Debug)
    # Magic enum
    target_include_directories(${PROJECT_NAME}
        PRIVATE src/libs/magic_enum/include
    )
endif()

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE ${Vulkan_INCLUDE_DIR}
    PUBLIC src/libs/glm
)

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    SDL2main
    SDL2-static
)

if(NOT ANDROID)
    target_link_libraries(${PROJECT_NAME}
        Vulkan::Vulkan
    )
endif()