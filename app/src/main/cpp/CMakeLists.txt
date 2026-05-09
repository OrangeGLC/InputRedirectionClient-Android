cmake_minimum_required(VERSION 3.22.1)

project("inputredirectionclient_android")

# Creates your game shared library. The name must be the same as the
# one used for loading in your Kotlin/Java or AndroidManifest.txt files.
add_library(inputredirectionclient_android SHARED
        JNIAdapt.cpp
        main.cpp
        Transmitter.cpp
        cJSON.c
)

# Searches for a package provided by the game activity dependency
find_package(game-activity REQUIRED CONFIG)
# Configure libraries CMake uses to link your target library.
target_link_libraries(inputredirectionclient_android
        # The game activity
        game-activity::game-activity
        jnigraphics
        android
        log)