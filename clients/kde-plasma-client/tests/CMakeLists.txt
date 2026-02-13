# RootStream KDE Plasma Client - Tests
cmake_minimum_required(VERSION 3.16)

# Find testing framework
find_package(Qt6 REQUIRED COMPONENTS Test)

# Enable testing
enable_testing()

# Test sources
set(TEST_SOURCES
    test_peermanager.cpp
    test_settingsmanager.cpp
)

# Renderer unit tests
if(ENABLE_RENDERER_OPENGL)
    list(APPEND TEST_SOURCES
        unit/test_renderer.cpp
    )
endif()

# Create test executable for each test file
foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    target_link_libraries(${TEST_NAME} PRIVATE
        Qt6::Test
        Qt6::Core
    )
    
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/renderer
    )
    
    # Link renderer library for renderer tests
    if(ENABLE_RENDERER_OPENGL AND ${TEST_SOURCE} MATCHES "renderer")
        target_sources(${TEST_NAME} PRIVATE
            ${CMAKE_SOURCE_DIR}/src/renderer/renderer.c
            ${CMAKE_SOURCE_DIR}/src/renderer/opengl_renderer.c
            ${CMAKE_SOURCE_DIR}/src/renderer/opengl_utils.c
            ${CMAKE_SOURCE_DIR}/src/renderer/color_space.c
            ${CMAKE_SOURCE_DIR}/src/renderer/frame_buffer.c
        )
        target_link_libraries(${TEST_NAME} PRIVATE
            OpenGL::GL
            ${X11_LIBRARIES}
        )
        target_include_directories(${TEST_NAME} PRIVATE
            ${X11_INCLUDE_DIR}
        )
    endif()
    
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

# Integration tests would go here
# add_subdirectory(integration)
