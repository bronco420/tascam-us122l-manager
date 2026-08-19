cmake_minimum_required(VERSION 3.16)
project(TascamUS122LManager VERSION 2.3.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt automoc for Q_OBJECT classes
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Options
option(BUILD_GUI "Build GUI (Qt6)" ON)
option(BUILD_STATIC "Build static binary" OFF)

# Dipendenze
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# JACK, PipeWire, ALSA via pkg-config (libraries don't ship CMake config files)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JACK REQUIRED IMPORTED_TARGET jack)
pkg_check_modules(PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)
pkg_check_modules(ALSA REQUIRED IMPORTED_TARGET alsa)

# Directory
set(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SRC_DIR "${PROJECT_DIR}/src")
set(CORE_DIR "${SRC_DIR}/core")
set(GUI_DIR "${SRC_DIR}/gui")
set(ASSETS_DIR "${PROJECT_DIR}/resources")

# Core sources
set(CORE_SOURCES
    "${CORE_DIR}/jack_manager.cpp"
    "${CORE_DIR}/pipewire_bridge.cpp"
    "${CORE_DIR}/sysmode.cpp"
    "${CORE_DIR}/mixer.cpp"
    "${CORE_DIR}/preset.cpp"
    "${CORE_DIR}/watchdog.cpp"
    "${CORE_DIR}/diagnostics.cpp"
    "${CORE_DIR}/config.cpp"
    "${CORE_DIR}/utils.cpp"
)

# GUI sources
set(GUI_SOURCES
    "${GUI_DIR}/main_window.cpp"
    "${GUI_DIR}/dashboard_widget.cpp"
    "${GUI_DIR}/mixer_widget.cpp"
    "${GUI_DIR}/preset_widget.cpp"
    "${GUI_DIR}/config_widget.cpp"
    "${GUI_DIR}/info_widget.cpp"
    "${GUI_DIR}/documentation_widget.cpp"
    "${GUI_DIR}/diagnostics_widget.cpp"
)

# Server sources
set(SERVER_SOURCES)

# Include directories
include_directories(
    ${SRC_DIR}
    ${CORE_DIR}
    ${GUI_DIR}
    ${ASSETS_DIR}
    ${JACK_INCLUDE_DIRS}
    ${PIPEWIRE_INCLUDE_DIRS}
    ${ALSA_INCLUDE_DIRS}
    ${Qt6_INCLUDE_DIRS}
)

# Sources
set(SOURCES
    "${SRC_DIR}/main.cpp"
    ${CORE_SOURCES}
    ${GUI_SOURCES}
    ${SERVER_SOURCES}
)

# Resources
qt6_add_resources(RESOURCES "${ASSETS_DIR}/resources.qrc")

# Executable
add_executable(tascam-us122l-manager
    ${SOURCES}
    ${RESOURCES}
)

# Link libraries
target_link_libraries(tascam-us122l-manager
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    PkgConfig::JACK
    PkgConfig::PIPEWIRE
    PkgConfig::ALSA
)

# Install
install(TARGETS tascam-us122l-manager
    RUNTIME DESTINATION bin
)

# Platform-specific settings
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    # systemd integration
    configure_file("${PROJECT_DIR}/cmake/tascam-us122l.service.in"
        "${CMAKE_BINARY_DIR}/tascam-us122l.service")
    install(FILES "${CMAKE_BINARY_DIR}/tascam-us122l.service"
        DESTINATION "${CMAKE_INSTALL_FULL_SYSCONFDIR}/systemd/user")
endif()

# Auto-start script
configure_file("${PROJECT_DIR}/cmake/autostart.sh.in"
    "${CMAKE_BINARY_DIR}/autostart.sh")
install(FILES "${CMAKE_BINARY_DIR}/autostart.sh"
    DESTINATION "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/tascam-us122l")

# Launcher script (subreaper for jackd)
install(PROGRAMS "${PROJECT_DIR}/scripts/tascam-jackd-launcher.py"
    DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/tascam-us122l")

# Desktop entry
configure_file("${PROJECT_DIR}/cmake/tascam-us122l-manager.desktop.in"
    "${CMAKE_BINARY_DIR}/tascam-us122l-manager.desktop")
install(FILES "${CMAKE_BINARY_DIR}/tascam-us122l-manager.desktop"
    DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/applications")

# Icons (install as tascam-us122l, no prefix)
install(FILES "${PROJECT_DIR}/resources/icons/tascam-us122l.svg"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps"
    RENAME tascam-us122l.svg)
install(FILES "${PROJECT_DIR}/resources/icons/tascam-us122l.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps"
    RENAME tascam-us122l.png)

# ============ Tests (CTest) ============
include(CTest)
if(BUILD_TESTING)
    add_executable(test_firmware
        "${PROJECT_DIR}/tests/test_firmware.cpp"
        "${CORE_DIR}/utils.cpp"
        "${CORE_DIR}/config.cpp"
    )
    target_include_directories(test_firmware PRIVATE
        ${SRC_DIR}
        ${CORE_DIR}
        ${Qt6_INCLUDE_DIRS}
    )
    target_link_libraries(test_firmware PRIVATE Qt6::Core)
    add_test(NAME firmware COMMAND test_firmware)
endif()