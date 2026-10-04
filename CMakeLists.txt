cmake_minimum_required(VERSION 3.25)
project(AVVPL VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
include(ExternalProject)
find_package(OpenMP)

# ── nlohmann/json ─────────────────────────────────────────────────────────────
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
  GIT_SHALLOW    TRUE
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# ── SDL3 ──────────────────────────────────────────────────────────────────────
FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG        release-3.2.14
  GIT_SHALLOW    TRUE
)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

# ── ThorVG — download source, build with Meson ────────────────────────────────
FetchContent_Declare(
  thorvg
  GIT_REPOSITORY https://github.com/thorvg/thorvg.git
  GIT_TAG        v0.15.10
  GIT_SHALLOW    TRUE
)
# Populate only: no CMakeLists.txt in thorvg, so don't add_subdirectory it.
FetchContent_GetProperties(thorvg)
if(NOT thorvg_POPULATED)
  FetchContent_Populate(thorvg)
endif()

set(THORVG_INSTALL_DIR "${CMAKE_BINARY_DIR}/thorvg_install")
set(THORVG_BUILD_DIR   "${CMAKE_BINARY_DIR}/thorvg_meson_build")

ExternalProject_Add(thorvg_ep
  SOURCE_DIR      "${thorvg_SOURCE_DIR}"
  BUILD_IN_SOURCE FALSE
  CONFIGURE_COMMAND
    meson setup
      "${THORVG_BUILD_DIR}"
      "${thorvg_SOURCE_DIR}"
      --prefix=${THORVG_INSTALL_DIR}
      --libdir=lib
      --default-library=static
      -Dengines=sw
      -Dloaders=svg,lottie,tvg,ttf
      -Dsavers=tvg
      -Dbindings=
      -Dexamples=false
      -Dtests=false
  BUILD_COMMAND
    ninja -C "${THORVG_BUILD_DIR}"
  INSTALL_COMMAND
    ninja -C "${THORVG_BUILD_DIR}" install
  BUILD_BYPRODUCTS
    "${THORVG_INSTALL_DIR}/lib/libthorvg.a"
)

file(MAKE_DIRECTORY "${THORVG_INSTALL_DIR}/include")

add_library(thorvg::thorvg STATIC IMPORTED GLOBAL)
add_dependencies(thorvg::thorvg thorvg_ep)
set_target_properties(thorvg::thorvg PROPERTIES
  IMPORTED_LOCATION             "${THORVG_INSTALL_DIR}/lib/libthorvg.a"
  INTERFACE_INCLUDE_DIRECTORIES "${THORVG_INSTALL_DIR}/include"
)

# ── App ───────────────────────────────────────────────────────────────────────
add_executable(${PROJECT_NAME}
  src/main.cpp
  src/ui/widget.cpp
  src/ui/wire_layer.cpp
  src/ui/slider.cpp
  src/ui/dial.cpp
  src/ui/histogram.cpp
  src/ui/node.cpp
  src/ui/node_canvas.cpp
  src/ui/graph_io.cpp
  src/ui/palette.cpp
  src/ui/tone_node.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  SDL3::SDL3-static
  thorvg::thorvg
  nlohmann_json::nlohmann_json
  $<$<BOOL:${OpenMP_CXX_FOUND}>:OpenMP::OpenMP_CXX>
)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()
