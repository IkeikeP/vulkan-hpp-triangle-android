# Project Info
cmake_minimum_required(VERSION 3.7)
cmake_policy(VERSION 3.7)
project(Naru VERSION 1.0.0 LANGUAGES CXX)

add_executable(${PROJECT_NAME})

# Sources directory
add_subdirectory(src)

# Dependencies
# Vulkan
find_package(Vulkan REQUIRED)
message("Vulkan found? " ${VULKAN_FOUND})
message("Vulkan_LIBRARY: " ${Vulkan_LIBRARY})
message("Vulkan_INCLUDE_DIR: " ${Vulkan_INCLUDE_DIR})  

# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(src/libs/glfw)


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