# Minimum CMake version
cmake_minimum_required(VERSION 3.22.0)

# Locate the Zephyr RTOS source
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# Name the project
project(blink)

# Locate the source code for the application
target_sources(app PRIVATE src/main.c)
