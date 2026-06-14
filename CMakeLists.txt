cmake_minimum_required(VERSION 3.20)
project(connecttebayo)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Qml Gui DBus)

qt_add_qml_module(connecttebayo
  URI connecttebayo
  VERSION 1.0
  SOURCES 
    connecttebayo.hpp
    connecttebayo.cpp
    networkmodel.hpp
    networkmodel.cpp
    iwdagent.hpp
    iwdagent.cpp
  NO_PLUGIN_OPTIONAL
  OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/connecttebayo"
)

target_link_libraries(connecttebayo PRIVATE
  Qt6::Core
  Qt6::Qml
  Qt6::Gui
  Qt6::DBus
)

set(QML_INSTALL_DIR lib/qt6/qml/connecttebayo)

install(TARGETS connecttebayo
  LIBRARY DESTINATION lib
)
install(TARGETS connecttebayoplugin
  LIBRARY DESTINATION  ${QML_INSTALL_DIR} 
)

install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/connecttebayo/"
  DESTINATION ${QML_INSTALL_DIR}
  FILES_MATCHING
  PATTERN "*.qmltypes"
  PATTERN "qmldir"
  PATTERN "*.qrc"
)
