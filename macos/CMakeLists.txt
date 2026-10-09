cmake_minimum_required(VERSION 3.20)
project(HammerillTrainerVC_macOS LANGUAGES C CXX)

if(NOT APPLE)
    message(FATAL_ERROR "This plugin targets the macOS (reVC) version of GTA Vice City.")
endif()

# The game executable is arm64-only, so the plugin must be too.
set(CMAKE_OSX_ARCHITECTURES arm64)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(GTAVC_APP "/Applications/GTA Vice City.app" CACHE PATH "GTA Vice City for macOS app bundle")

# The plugin uses the game's own GLFW. Linking against that exact dylib makes
# the reference resolve to the copy the game already loaded.
set(GAME_GLFW "${GTAVC_APP}/Contents/Frameworks/libglfw.3.dylib")
if(NOT EXISTS "${GAME_GLFW}")
    message(FATAL_ERROR "Couldn't find ${GAME_GLFW}.\n"
                        "Pass the game's location with -DGTAVC_APP=\"/path/to/GTA Vice City.app\".")
endif()

include(FetchContent)
FetchContent_Declare(imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.9b.tar.gz
    URL_HASH SHA256=8e1bbc76c71d74fef2fb85db7e7ca8eba13d6a86623c54992b60162db554ffdb
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
# Only GLFW's headers are needed; SOURCE_SUBDIR points at a folder without a
# CMakeLists.txt so the library itself isn't built.
FetchContent_Declare(glfw
    URL https://github.com/glfw/glfw/archive/refs/tags/3.4.tar.gz
    URL_HASH SHA256=c038d34200234d071fae9345bc455e4a8f2f544ab60150765d7704e08f3dac01
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR include)
FetchContent_MakeAvailable(imgui glfw)

# Data lists and font from resource/, compiled into the plugin.
set(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../resource")
set(EMBEDDED
    "vehicles.toml=${RESOURCE_DIR}/GrinchTrainerVC/data/vehicles.toml"
    "weapons.toml=${RESOURCE_DIR}/GrinchTrainerVC/data/weapons.toml"
    "locations.toml=${RESOURCE_DIR}/GrinchTrainerVC/data/locations.toml"
    "text.ttf=${RESOURCE_DIR}/fonts_raw/text.ttf")
set(EMBEDDED_SOURCES "")
foreach(item IN LISTS EMBEDDED)
    string(REGEX REPLACE "^[^=]*=" "" path "${item}")
    list(APPEND EMBEDDED_SOURCES "${path}")
endforeach()
string(REPLACE ";" "," EMBEDDED_ARG "${EMBEDDED}")
set(EMBEDDED_CPP "${CMAKE_CURRENT_BINARY_DIR}/embedded_files.cpp")
add_custom_command(
    OUTPUT "${EMBEDDED_CPP}"
    COMMAND "${CMAKE_COMMAND}" "-DOUTPUT=${EMBEDDED_CPP}" "-DFILES=${EMBEDDED_ARG}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedFiles.cmake"
    DEPENDS ${EMBEDDED_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedFiles.cmake"
    COMMENT "Embedding trainer data files"
    VERBATIM)

add_library(HammerillTrainerVC SHARED
    src/plugin.cpp
    src/game.cpp
    src/trainer.cpp
    src/data.cpp
    "${EMBEDDED_CPP}"
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp")

target_include_directories(HammerillTrainerVC PRIVATE
    src
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends"
    "${glfw_SOURCE_DIR}/include")
target_compile_definitions(HammerillTrainerVC PRIVATE GL_SILENCE_DEPRECATION IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
target_compile_options(HammerillTrainerVC PRIVATE -fvisibility=hidden)
target_link_libraries(HammerillTrainerVC PRIVATE "${GAME_GLFW}" "-framework OpenGL")
set_target_properties(HammerillTrainerVC PROPERTIES
    PREFIX ""
    OUTPUT_NAME HammerillTrainerVC
    SUFFIX ".dylib"
    MACOSX_RPATH OFF
    BUILD_WITH_INSTALL_NAME_DIR ON
    INSTALL_NAME_DIR "@loader_path")
