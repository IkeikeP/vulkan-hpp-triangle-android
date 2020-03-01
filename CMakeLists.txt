# Project Info
cmake_minimum_required(VERSION 3.7)
cmake_policy(VERSION 3.7)
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

# Dependencies
# Vulkan
find_package(Vulkan REQUIRED)
if(${VULKAN_FOUND})
    message("Vulkan_LIBRARY: " ${Vulkan_LIBRARY})
    message("Vulkan_INCLUDE_DIR: " ${Vulkan_INCLUDE_DIR}) 
else() 
    message(FATAL_ERROR "Vulkan not found, please verify if vulkan sdk is installed.")
endif()

# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(src/libs/glfw)

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
    glfw
)