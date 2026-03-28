cmake_minimum_required(VERSION 3.16)

project(DynamicalSystems VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Require Qt6 Widgets and SvgWidgets
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets SvgWidgets)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    systems.h
    welcomewindow.cpp
    welcomewindow.h
    detailsdialog.h
    lorenzdialog.cpp
    lorenzdialog.h
    rosslerdialog.cpp
    rosslerdialog.h
    vanderpoldialog.cpp
    vanderpoldialog.h
    doublependulumdialog.cpp
    doublependulumdialog.h
    images.qrc
)

qt_add_executable(DynamicalSystems
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    helpdialog.h helpdialog.cpp
    initialconditionsdialog.h initialconditionsdialog.cpp

)

target_link_libraries(DynamicalSystems
    PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::SvgWidgets
)

# macOS/iOS bundle settings
set_target_properties(DynamicalSystems PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.DynamicalSystems
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS DynamicalSystems
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(DynamicalSystems)
