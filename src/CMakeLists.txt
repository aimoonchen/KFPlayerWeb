cmake_minimum_required(VERSION 3.6)

project(MY_APP)

# find_library(SDL2 SDL2)

# set(INCLUDE_LOCATION ../SDL/include ../bgfx.cmake/bgfx/include ../bgfx.cmake/bx/include ../bgfx.cmake/bimg/include)
# include_directories(${INCLUDE_LOCATION})
set(URHO3D_SDK_PATH D:/Github/rbfx/SDK)
# set(URHO3D_SDK_PATH D:/Github/rbfx/build_web)
set(INCLUDE_LOCATION ../bgfx.cmake/bgfx/include ../bgfx.cmake/bx/include ../bgfx.cmake/bimg/include ${URHO3D_SDK_PATH}/include ${URHO3D_SDK_PATH}/include/Urho3D/ThirdParty)
include_directories(${INCLUDE_LOCATION})
# link_directories(D:/Github/rbfx/build_web/lib)

# add_library(main SHARED)
# target_sources(main PRIVATE main.cpp)

# if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    # set(USE_C_FLAGS "-s USE_SDL=2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_C_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_C_FLAGS}")
    set(USE_L_FLAGS "-s FETCH=1 -s MAX_WEBGL_VERSION=2 -s MIN_WEBGL_VERSION=2 -s FULL_ES3=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] --preload-file D:/Github/rbfx/binweb@/")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_L_FLAGS}")
    add_executable(main)
    target_compile_definitions(main
        PUBLIC  -DEASTL_OPENSOURCE=1
                -DEASTL_RTTI_ENABLED=0
                -DEASTL_URHO3D_EXTENSIONS=1
                -DEASTL_SIZE_T_32BIT=1        # Urho3D uses `unsigned` for sizes all over place. Can not use native size without breaking lots of code.
                -DEASTDC_GLOBALPTR_SUPPORT_ENABLED=0
                -DEASTDC_THREADING_SUPPORTED=0
                -DEASTL_STD_ITERATOR_CATEGORY_ENABLED=1
                # -DEASTL_DEBUG=$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>
                -DEASTL_DEBUG=0
        PRIVATE -D_CHAR16T
    )
    # target_sources(main PRIVATE bgfx_utils.cpp cmd.cpp entry.cpp entry_sdl.cpp input.cpp main.cpp)
    target_sources(main PRIVATE entry.cpp entry_html5.cpp helloworld.cpp)
    # target_link_libraries(main bgfx bx bimg SDL2)
    # set(URHO3D_LIBS )
    target_link_libraries(main bgfx bx bimg
        ${URHO3D_SDK_PATH}/lib/libUrho3D.a
        ${URHO3D_SDK_PATH}/lib/libastc-codec.a
        ${URHO3D_SDK_PATH}/lib/libLua.a
        ${URHO3D_SDK_PATH}/lib/libEffekseer.a
        ${URHO3D_SDK_PATH}/lib/libBox2D.a
        ${URHO3D_SDK_PATH}/lib/libBullet.a
        ${URHO3D_SDK_PATH}/lib/libDetour.a
        ${URHO3D_SDK_PATH}/lib/libDetourCrowd.a
        ${URHO3D_SDK_PATH}/lib/libDetourTileCache.a
        ${URHO3D_SDK_PATH}/lib/libEASTL.a
        ${URHO3D_SDK_PATH}/lib/libETCPACK.a
        ${URHO3D_SDK_PATH}/lib/libfmt.a
        ${URHO3D_SDK_PATH}/lib/libFreeType.a
        ${URHO3D_SDK_PATH}/lib/libik.a
        ${URHO3D_SDK_PATH}/lib/libLZ4.a
        ${URHO3D_SDK_PATH}/lib/libPugiXml.a
        ${URHO3D_SDK_PATH}/lib/libRecast.a
        ${URHO3D_SDK_PATH}/lib/libRmlUi.a
        ${URHO3D_SDK_PATH}/lib/libSDL.a
        ${URHO3D_SDK_PATH}/lib/libStanHull.a
        ${URHO3D_SDK_PATH}/lib/libtinygltf.a
        ${URHO3D_SDK_PATH}/lib/libWebP.a
        ${URHO3D_SDK_PATH}/lib/libSLikeNet.a
        ${URHO3D_SDK_PATH}/lib/fmodstudio.a
        ${URHO3D_SDK_PATH}/lib/fmod.a
    )
# elseif()
    # add_library(main SHARED)
    # target_sources(main PRIVATE bgfx_utils.cpp cmd.cpp entry.cpp entry_sdl.cpp input.cpp main.cpp)
    # target_link_libraries(main EGL GLESv3 android log bgfx bx bimg SDL2)
# endif()