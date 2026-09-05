cmake_minimum_required(VERSION 3.16)
project(PodSwitch VERSION 1.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# OBS Studio
#   Option A (system install, e.g. installed via OBS PPA on Linux):
#       find_package(libobs) works automatically.
#   Option B (CI / source only):
#       -DOBS_SOURCE_DIR=/path/to/obs-studio
#       We include our bundled src/obs-config.h stub so no OBS cmake build needed.
# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# OBS Studio - Forced Header-Only Build (Option B)
# ---------------------------------------------------------------------------
if(NOT OBS_SOURCE_DIR)
  set(OBS_SOURCE_DIR "" CACHE PATH "obs-studio source tree")
endif()
if(NOT OBS_CONFIG_DIR)
  set(OBS_CONFIG_DIR "${OBS_SOURCE_DIR}/build_universal/config" CACHE PATH "obs-studio build/config dir")
endif()

message(STATUS "[PodSwitch] Using dependencies from: ${OBS_SOURCE_DIR}")

add_library(OBS::libobs INTERFACE IMPORTED)
target_include_directories(OBS::libobs INTERFACE
  "${OBS_SOURCE_DIR}/libobs"
  "${OBS_CONFIG_DIR}"
  "${CMAKE_SOURCE_DIR}/src"    # our bundled obs-config.h stub lives here
)

add_library(OBS::obs-frontend-api INTERFACE IMPORTED)
target_include_directories(OBS::obs-frontend-api INTERFACE
  "${OBS_SOURCE_DIR}/UI/obs-frontend-api"
  "${OBS_SOURCE_DIR}/libobs"
  "${OBS_CONFIG_DIR}"
  "${CMAKE_SOURCE_DIR}/src"
)

# Plugin resolves OBS symbols at load time — no link-time OBS libs needed
set(OBS_SKIP_LINK TRUE)


# ---------------------------------------------------------------------------
# Qt (Qt6 → Qt5 → headless)
# ---------------------------------------------------------------------------
find_package(Qt6 COMPONENTS Core Widgets QUIET)
if(Qt6_FOUND)
  set(QT_NS Qt6)
else()
  find_package(Qt5 COMPONENTS Core Widgets QUIET)
  if(Qt5_FOUND)
    set(QT_NS Qt5)
  else()
    message(WARNING "[podswitch] Qt not found — building headless (no UI)")
    set(QT_NS "")
  endif()
endif()

if(QT_NS)
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTOUIC ON)
  set(CMAKE_AUTORCC ON)
endif()

# Removed invalid macro

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(SRCS
  src/plugin-main.cpp
  src/audio-monitor.cpp
  src/switch-engine.cpp
  src/config.cpp
  src/utils.cpp
  src/scene-generator.cpp
  src/motion-detector.cpp
)
if(QT_NS)
  list(APPEND SRCS src/ui/dock-widget.cpp src/ui/settings-dialog.cpp)
endif()

add_library(podswitch MODULE ${SRCS})
target_include_directories(podswitch PRIVATE src src/ui)
target_link_libraries(podswitch PRIVATE OBS::libobs OBS::obs-frontend-api)
if(QT_NS)
  target_link_libraries(podswitch PRIVATE ${QT_NS}::Core ${QT_NS}::Widgets)
endif()
set_target_properties(podswitch PROPERTIES PREFIX "")

# Runtime-link OBS symbols (resolved at plugin load time by OBS)
if(OBS_SKIP_LINK)
  if(APPLE)
    target_link_options(podswitch PRIVATE -undefined dynamic_lookup)
  elseif(UNIX)
    target_link_options(podswitch PRIVATE -Wl,--allow-shlib-undefined)
  endif()
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
if(APPLE)
  set(_PLUG "$ENV{HOME}/Library/Application Support/obs-studio/plugins/podswitch.plugin")
elseif(WIN32)
  set(_PLUG "$ENV{APPDATA}/obs-studio/plugins/podswitch")
else()
  set(_PLUG "$ENV{HOME}/.config/obs-studio/plugins/podswitch")
endif()
if(APPLE)
  install(TARGETS podswitch LIBRARY DESTINATION "${_PLUG}/Contents/MacOS")
  install(DIRECTORY data/ DESTINATION "${_PLUG}/Contents/Resources")
else()
  install(TARGETS podswitch LIBRARY DESTINATION "${_PLUG}/bin/64bit")
  install(DIRECTORY data/ DESTINATION "${_PLUG}/data")
endif()

if(APPLE AND QT_NS)
  add_custom_command(TARGET podswitch POST_BUILD
    COMMAND sh "${CMAKE_SOURCE_DIR}/tools/fix_macos_qt.sh" $<TARGET_FILE:podswitch>
    COMMENT "Fixing macOS Qt frameworks linking to use OBS internal Frameworks"
  )
endif()
