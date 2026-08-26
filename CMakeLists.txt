cmake_minimum_required(VERSION 3.21)

project(DynamicalSystemsVisualizer VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable automatic Qt features
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Svg
    SvgWidgets
)
qt_standard_project_setup()

qt_add_executable(DynamicalSystemsVisualizer
    main.cpp

    mainwindow.h
    mainwindow.cpp

    # Original headers
    systems.h

    # Dialogs from your repo
    helpdialog.h
    helpdialog.cpp

    initialconditionsdialog.h
    initialconditionsdialog.cpp

    lorenzdialog.h
    lorenzdialog.cpp

    rosslerdialog.h
    rosslerdialog.cpp

    vanderpoldialog.h
    vanderpoldialog.cpp

    doublependulumdialog.h
    doublependulumdialog.cpp

    welcomewindow.h
    welcomewindow.cpp

    # Resource file from your repo
    images.qrc
)

target_link_libraries(DynamicalSystemsVisualizer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    Qt6::SvgWidgets
)

if(MSVC)
    target_compile_options(DynamicalSystemsVisualizer PRIVATE
        /W4
        /permissive-
    )
else()
    target_compile_options(DynamicalSystemsVisualizer PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# macOS bundle config
if(APPLE)
    set_target_properties(DynamicalSystemsVisualizer PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Dynamical Systems Visualizer"
    )
endif()

install(TARGETS DynamicalSystemsVisualizer
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
