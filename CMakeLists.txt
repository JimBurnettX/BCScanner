cmake_minimum_required(VERSION 3.16)
project(BCScan VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Try Qt6 first, fall back to Qt5
find_package(Qt6 COMPONENTS Core Gui Widgets SerialPort QUIET)
if(Qt6_FOUND)
    set(QT Qt6)
    message(STATUS "Using Qt6")
else()
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets SerialPort)
    set(QT Qt5)
    message(STATUS "Using Qt5")
endif()

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/lcdwidget.cpp
    src/scannerserial.cpp
    src/programwindow.cpp
    src/settingswindow.cpp
    src/appsettings.cpp
    src/transmissionlogger.cpp
    src/sqlpoller.cpp
    src/stspoller.cpp
    src/debugwindow.cpp
)

set(HEADERS
    src/mainwindow.h
    src/lcdwidget.h
    src/scannerserial.h
    src/programwindow.h
    src/settingswindow.h
    src/appsettings.h
    src/transmissionlogger.h
    src/ctcssdcsdata.h
    src/sqlpoller.h
    src/stspoller.h
    src/debugwindow.h
)

if(WIN32)
    add_executable(bcscan WIN32 ${SOURCES} ${HEADERS})
elseif(APPLE)
    add_executable(bcscan MACOSX_BUNDLE ${SOURCES} ${HEADERS})
else()
    add_executable(bcscan ${SOURCES} ${HEADERS})
endif()

target_link_libraries(bcscan PRIVATE
    ${QT}::Core
    ${QT}::Gui
    ${QT}::Widgets
    ${QT}::SerialPort
)

target_include_directories(bcscan PRIVATE src)

# Install rules (required by CPack)
install(TARGETS bcscan RUNTIME DESTINATION bin)

if(UNIX)
    install(FILES assets/bcscan.svg
        DESTINATION share/icons/hicolor/scalable/apps)
    install(FILES assets/bcscan.desktop
        DESTINATION share/applications)
endif()

if(WIN32)
    get_target_property(_qmake_exe ${QT}::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_exe}" DIRECTORY)
    install(CODE "
        execute_process(COMMAND
            \"${_qt_bin_dir}/windeployqt.exe\"
            --no-translations
            --no-system-d3d-compiler
            --no-opengl-sw
            \"\${CMAKE_INSTALL_PREFIX}/bin/bcscan.exe\"
        )
    ")
endif()

# Packaging
set(CPACK_PACKAGE_NAME "bcscan")
set(CPACK_PACKAGE_VENDOR "James Burnett")
set(CPACK_PACKAGE_CONTACT "jamesburnettio@proton.me")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Desktop controller for the Uniden BC125AT scanner")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})

# Debian
if(NOT WIN32)
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
endif()
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

# RPM
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase >= 6.0, qt6-qtserialport >= 6.0")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Utilities")

# NSIS (Windows)
set(CPACK_NSIS_DISPLAY_NAME "BCScan")
set(CPACK_NSIS_PACKAGE_NAME "BCScan")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
set(CPACK_NSIS_EXECUTABLES_DIRECTORY "bin")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_CREATE_ICONS_EXTRA
    "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\BCScan.lnk' '$INSTDIR\\\\bin\\\\bcscan.exe'")
set(CPACK_NSIS_DELETE_ICONS_EXTRA
    "Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\BCScan.lnk'")

include(CPack)
