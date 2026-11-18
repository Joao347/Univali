cmake_minimum_required(VERSION 3.10)
project(MiniFileSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(mini_file_system
    main.cpp
    src/MiniFileSystem.cpp
    src/DiscoSimulado.cpp
)

target_include_directories(mini_file_system PRIVATE include)
