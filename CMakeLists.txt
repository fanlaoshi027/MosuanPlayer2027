cmake_minimum_required(VERSION 3.25)
project(MosuanPlayer2027 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)

qt_add_executable(MosuanPlayer
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/MpvEngine.cpp
    src/MpvEngine.h
)

target_link_libraries(MosuanPlayer PRIVATE Qt6::Widgets Qt6::Concurrent)
target_compile_definitions(MosuanPlayer PRIVATE QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII)

if(MSVC)
    target_compile_options(MosuanPlayer PRIVATE /W4 /permissive- /utf-8)
endif()
