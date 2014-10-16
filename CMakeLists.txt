project(papatextureeditor)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 COMPONENTS QtCore QtGui REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set(papatextureeditor helpdialog.cpp papafile.cpp texturelistmodel.cpp papatextureeditor.cpp main.cpp)
qt4_automoc(${papatextureeditor})
add_executable(papatextureeditor ${papatextureeditor})
if(WIN32)
	target_link_libraries(papatextureeditor ${QT_QTGUI_LIBRARY} ${QT_QTCORE_LIBRARY} -mwindows -lws2_32 -ljpeg -lpng -ltiff -llzma -lmng -lz -limm32 -llcms -lwinmm)
else()
	target_link_libraries(papatextureeditor ${QT_QTGUI_LIBRARY} ${QT_QTCORE_LIBRARY})
endif()

install(TARGETS papatextureeditor RUNTIME DESTINATION bin)
