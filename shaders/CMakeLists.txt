function(compile_shaders_to_spirv SHADERS)
    find_program(GLSLANG glslangValidator HINTS "$ENV{VULKAN_SDK}/bin" REQUIRED)
    find_program(SPIRV_OPT spirv-opt HINTS "$ENV{VULKAN_SDK}/bin" REQUIRED)

    foreach(SHADER ${SHADERS})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(COMPILE_OUTPUT "${SHADER_NAME}.debug.spv")
        add_custom_command(OUTPUT ${COMPILE_OUTPUT} 
                           COMMAND ${GLSLANG} -V ${SHADER} -o ${COMPILE_OUTPUT}
                           DEPENDS ${SHADER}
                           COMMENT "Validating & Compiling shader")
        set(OPTIMIZE_OUTPUT "${SHADER_NAME}.spv")
        add_custom_command(OUTPUT ${OPTIMIZE_OUTPUT} 
                           COMMAND ${SPIRV_OPT} -O ${COMPILE_OUTPUT} -o ${OPTIMIZE_OUTPUT} 
                           DEPENDS ${COMPILE_OUTPUT}
                           COMMENT "Optimizing SPIR-V")
        list(APPEND SPIRVS ${OPTIMIZE_OUTPUT})
    endforeach()
    set(COMPILED_SHADERS "${SPIRVS}" PARENT_SCOPE)
endfunction()

set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
file(GLOB_RECURSE SHADERS
     ${SHADER_DIR}/*.vert
     ${SHADER_DIR}/*.frag
     ${SHADER_DIR}/*.tesc
     ${SHADER_DIR}/*.geom)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
             PREFIX "Naru\\Shaders"
             FILES ${SHADERS})
compile_shaders_to_spirv("${SHADERS}")
source_group("Naru\\Shaders\\Bin" FILES ${COMPILED_SHADERS})
add_custom_target(shaders DEPENDS ${COMPILED_SHADERS})

add_dependencies(${PROJECT_NAME} shaders)
if (ANDROID)
    list(GET ANDROID_ASSETS_DIRECTORIES 0 first-android-assets-dir)
	message(STATUS "Android Assets Directory : ${ANDROID_ASSETS_DIRECTORIES}")
    set (source "${CMAKE_CURRENT_BINARY_DIR}")
    set (destination "${first-android-assets-dir}\\shaders")
    add_custom_command(
        TARGET shaders POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${source} ${destination} 
        DEPENDS ${destination}
        COMMENT "symbolic link resources folder from ${source} => ${destination}"
    )
endif()