cmake_minimum_required(VERSION 3.10)
project(D3D11_Storage_Test)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
add_executable(${PROJECT_NAME}
    main.cpp
    test.cpp
    texture_as_buffer.cpp
    d3d11_helper.cpp
)

# Add Windows-specific libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        d3d11
        d3dcompiler
        dxgi
    )
endif()

# Copy shader file to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    COMMENT "Copying shaders directory to build directory"
)
