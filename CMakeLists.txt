cmake_minimum_required(VERSION 3.6)
#SET (CMAKE_C_COMPILER_WORKS TRUE)
#SET (CMAKE_CXX_COMPILER_WORKS TRUE)
# Set C++ standard
if (NOT DEFINED CMAKE_CXX_STANDARD)
    set (CMAKE_CXX_STANDARD 17)
endif ()

project(GAME)

# armeabi-v7a requires cpufeatures library
# include(AndroidNdkModules)
# android_ndk_import_module_cpufeatures()


# SDL sources are in a subfolder named "SDL"
# add_subdirectory(SDL)
add_subdirectory(bgfx.cmake)
# Compilation of companion libraries
#add_subdirectory(SDL_image)
#add_subdirectory(SDL_mixer)
#add_subdirectory(SDL_ttf)

# Your game and its CMakeLists.txt are in a subfolder named "src"
add_subdirectory(src)

