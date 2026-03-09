cmake_minimum_required(VERSION 3.20)
project(MiniCode VERSION 1.0.0 LANGUAGES CXX)

# ============================================================
# C++20 standard
# ============================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# AUTOMOC must be set BEFORE find_package for Qt
# ============================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ============================================================
# Build type
# ============================================================
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# ============================================================
# Find Qt6 (fallback to Qt5)
# ============================================================
find_package(Qt6 COMPONENTS Core Widgets Gui Network QUIET)

if(Qt6_FOUND)
    message(STATUS "Qt6 found: ${Qt6_VERSION}")
    set(QT_LIBS Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Network)
else()
    find_package(Qt5 COMPONENTS Core Widgets Gui Network REQUIRED)
    message(STATUS "Qt5 found: ${Qt5_VERSION}")
    set(QT_LIBS Qt5::Core Qt5::Widgets Qt5::Gui Qt5::Network)
endif()

# ============================================================
# Source files — use absolute paths via CMAKE_SOURCE_DIR
# ============================================================
set(SOURCES
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/editor/Editor.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/MainWindow.cpp
    ${CMAKE_SOURCE_DIR}/src/syntax/SyntaxHighlighter.cpp
    ${CMAKE_SOURCE_DIR}/src/parser/CodeParser.cpp
    ${CMAKE_SOURCE_DIR}/src/lsp/LSPClient.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/FileManager.cpp
)

set(HEADERS
    ${CMAKE_SOURCE_DIR}/src/editor/Editor.hpp
    ${CMAKE_SOURCE_DIR}/src/ui/MainWindow.hpp
    ${CMAKE_SOURCE_DIR}/src/syntax/SyntaxHighlighter.hpp
    ${CMAKE_SOURCE_DIR}/src/parser/CodeParser.hpp
    ${CMAKE_SOURCE_DIR}/src/lsp/LSPClient.hpp
    ${CMAKE_SOURCE_DIR}/src/utils/FileManager.hpp
)

# ============================================================
# Executable
# ============================================================
add_executable(MiniCode ${SOURCES} ${HEADERS})

target_include_directories(MiniCode PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(MiniCode PRIVATE ${QT_LIBS})

# ============================================================
# Compiler warnings
# ============================================================
target_compile_options(MiniCode PRIVATE
    -Wall -Wextra -Wpedantic
    -Wno-unused-parameter
)

# ============================================================
# Install
# ============================================================
install(TARGETS MiniCode DESTINATION bin)

message(STATUS "MiniCode configured. Build with: cmake --build .")
