# Project Info
cmake_minimum_required(VERSION 3.16)
cmake_policy(VERSION 3.16)
project(Naru VERSION 1.0.0 LANGUAGES CXX)
add_executable(${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

# Build type
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release ... FORCE )
endif()
if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=1)
endif()
message("Build type: ${CMAKE_BUILD_TYPE}")

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
find_package(Vulkan REQUIRED)
if(${VULKAN_FOUND})
    message("Vulkan_LIBRARY: " ${Vulkan_LIBRARY})
    message("Vulkan_INCLUDE_DIR: " ${Vulkan_INCLUDE_DIR}) 
else() 
    message(FATAL_ERROR "Vulkan not found, please verify if Vulkan SDK is installed.")
endif()

# sdl
#include_directories(src/libs/sdl/include)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
add_subdirectory(src/libs/sdl)

# Debug
if(CMAKE_BUILD_TYPE MATCHES Debug)
    # Magic enum
    #add_subdirectory(src/libs/magic_enum)
    target_include_directories(${PROJECT_NAME}
        PRIVATE src/libs/magic_enum/include
    )
endif()

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE Vulkan::Vulkan
    PUBLIC src/libs/glm
)

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    Vulkan::Vulkan
    SDL2main
    SDL2-static
)