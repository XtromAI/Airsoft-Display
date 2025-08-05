# Pico SDK compatible CMakeLists.txt for Adafruit_SH110X
cmake_minimum_required(VERSION 3.13)

add_library(Adafruit_SH110X STATIC
    Adafruit_SH1106G.cpp
    Adafruit_SH1107.cpp
    Adafruit_SH110X.cpp
)

target_include_directories(Adafruit_SH110X PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link with Adafruit_GFX as a dependency
target_link_libraries(Adafruit_SH110X PUBLIC
    Adafruit-GFX-Library
)
