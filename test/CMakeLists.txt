cmake_minimum_required(VERSION 3.10)

set(BINARY ${CMAKE_PROJECT_NAME}_tst)

# file(GLOB_RECURSE TEST_SOURCES LIST_DIRECTORIES false *.hpp *.cpp)
set(TEST_SOURCES 
    example.cpp
    main.cpp)

include_directories(.)

# Below line seemed to cause issues with collecting coverage from source
# set(SOURCES ${TEST_SOURCES})

add_executable(${BINARY} ${TEST_SOURCES})

add_test(NAME ${BINARY} COMMAND ${BINARY})

target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_lib gtest DaisySP)