project(papatextureeditor)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set(papatextureeditor papafile.cpp texturelistmodel.cpp papatextureeditor.cpp main.cpp)
qt4_automoc(${papatextureeditor})
add_executable(papatextureeditor ${papatextureeditor})
target_link_libraries(papatextureeditor ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTOPENGL_LIBRARY})
install(TARGETS papatextureeditor RUNTIME DESTINATION bin)
