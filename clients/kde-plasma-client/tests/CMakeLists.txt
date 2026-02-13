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

# Vulkan renderer unit tests
if(ENABLE_RENDERER_VULKAN)
    list(APPEND TEST_SOURCES
        unit/test_vulkan_renderer.cpp
    )
endif()

# Proton renderer unit tests
if(ENABLE_RENDERER_PROTON)
    list(APPEND TEST_SOURCES
        unit/test_proton_renderer.cpp
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
        target_link_libraries(${TEST_NAME} PRIVATE OpenGL::GL ${X11_LIBRARIES})
        target_compile_definitions(${TEST_NAME} PRIVATE HAVE_OPENGL_RENDERER)
    endif()
    
    # Link Vulkan renderer for Vulkan tests
    if(ENABLE_RENDERER_VULKAN AND ${TEST_SOURCE} MATCHES "vulkan")
        target_sources(${TEST_NAME} PRIVATE
            ${CMAKE_SOURCE_DIR}/src/renderer/renderer.c
            ${CMAKE_SOURCE_DIR}/src/renderer/vulkan_renderer.c
            ${CMAKE_SOURCE_DIR}/src/renderer/vulkan_wayland.c
            ${CMAKE_SOURCE_DIR}/src/renderer/vulkan_x11.c
            ${CMAKE_SOURCE_DIR}/src/renderer/vulkan_headless.c
            ${CMAKE_SOURCE_DIR}/src/renderer/color_space.c
            ${CMAKE_SOURCE_DIR}/src/renderer/frame_buffer.c
        )
        target_link_libraries(${TEST_NAME} PRIVATE Vulkan::Vulkan ${X11_LIBRARIES})
        if(WAYLAND_FOUND)
            target_link_libraries(${TEST_NAME} PRIVATE ${WAYLAND_LIBRARIES})
        endif()
        target_compile_definitions(${TEST_NAME} PRIVATE HAVE_VULKAN_RENDERER)
    endif()
    
    # Link Proton renderer for Proton tests
    if(ENABLE_RENDERER_PROTON AND ${TEST_SOURCE} MATCHES "proton")
        target_sources(${TEST_NAME} PRIVATE
            ${CMAKE_SOURCE_DIR}/src/renderer/proton_detector.c
            ${CMAKE_SOURCE_DIR}/src/renderer/proton_renderer.c
            ${CMAKE_SOURCE_DIR}/src/renderer/dxvk_interop.c
            ${CMAKE_SOURCE_DIR}/src/renderer/vkd3d_interop.c
            ${CMAKE_SOURCE_DIR}/src/renderer/proton_game_db.c
            ${CMAKE_SOURCE_DIR}/src/renderer/proton_settings.c
        )
        # Proton renderer depends on Vulkan
        target_sources(${TEST_NAME} PRIVATE
            ${CMAKE_SOURCE_DIR}/src/renderer/renderer.c
            ${CMAKE_SOURCE_DIR}/src/renderer/vulkan_renderer.c
            ${CMAKE_SOURCE_DIR}/src/renderer/vulkan_wayland.c
            ${CMAKE_SOURCE_DIR}/src/renderer/vulkan_x11.c
            ${CMAKE_SOURCE_DIR}/src/renderer/vulkan_headless.c
            ${CMAKE_SOURCE_DIR}/src/renderer/color_space.c
            ${CMAKE_SOURCE_DIR}/src/renderer/frame_buffer.c
        )
        target_link_libraries(${TEST_NAME} PRIVATE Vulkan::Vulkan ${X11_LIBRARIES})
        if(WAYLAND_FOUND)
            target_link_libraries(${TEST_NAME} PRIVATE ${WAYLAND_LIBRARIES})
        endif()
        target_compile_definitions(${TEST_NAME} PRIVATE HAVE_PROTON_RENDERER HAVE_VULKAN_RENDERER)
    endif()
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
        target_compile_definitions(${TEST_NAME} PRIVATE HAVE_OPENGL_RENDERER)
    endif()
    
    # Link Vulkan renderer for Vulkan tests
    if(ENABLE_RENDERER_VULKAN AND ${TEST_SOURCE} MATCHES "vulkan")
        target_sources(${TEST_NAME} PRIVATE
            ${CMAKE_SOURCE_DIR}/src/renderer/renderer.c
            ${CMAKE_SOURCE_DIR}/src/renderer/vulkan_renderer.c
            ${CMAKE_SOURCE_DIR}/src/renderer/vulkan_wayland.c
            ${CMAKE_SOURCE_DIR}/src/renderer/vulkan_x11.c
            ${CMAKE_SOURCE_DIR}/src/renderer/vulkan_headless.c
            ${CMAKE_SOURCE_DIR}/src/renderer/color_space.c
            ${CMAKE_SOURCE_DIR}/src/renderer/frame_buffer.c
        )
        target_link_libraries(${TEST_NAME} PRIVATE
            Vulkan::Vulkan
            ${X11_LIBRARIES}
        )
        target_include_directories(${TEST_NAME} PRIVATE
            ${X11_INCLUDE_DIR}
            ${Vulkan_INCLUDE_DIRS}
        )
        if(WAYLAND_FOUND)
            target_link_libraries(${TEST_NAME} PRIVATE ${WAYLAND_LIBRARIES})
            target_include_directories(${TEST_NAME} PRIVATE ${WAYLAND_INCLUDE_DIRS})
        endif()
        target_compile_definitions(${TEST_NAME} PRIVATE HAVE_VULKAN_RENDERER)
    endif()
    
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

# Integration tests would go here
# add_subdirectory(integration)
