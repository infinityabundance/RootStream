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
    )
    
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

# Integration tests would go here
# add_subdirectory(integration)
