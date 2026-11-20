cmake_minimum_required(VERSION 3.21)

project(Cineginsa VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)      # génère ui_mainwindow.h à partir de mainwindow.ui
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Quick Multimedia)

qt_add_executable(Cineginsa
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    regie.cpp
    regie.h
)

# display.qml est embarqué dans l'exécutable -> "qrc:/display.qml"
qt_add_resources(Cineginsa "qml"
    PREFIX "/"
    FILES display.qml
)

target_link_libraries(Cineginsa PRIVATE Qt6::Widgets Qt6::Quick Qt6::Multimedia)

set_target_properties(Cineginsa PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Cineginsa
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
