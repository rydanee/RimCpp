cmake_minimum_required(VERSION 3.15)
project(RimCpp)

include(FetchContent)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG master
)

FetchContent_MakeAvailable(raylib)

find_package(raylib REQUIRED)

add_compile_definitions(SUPPORT_FILEFORMAT_JPG)

add_executable(${PROJECT_NAME} src/main.cpp)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
