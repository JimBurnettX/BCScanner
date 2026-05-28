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
