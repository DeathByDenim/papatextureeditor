project(papatexturereader)
cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set(papatexturereader_SRCS papatexturereader.cpp main.cpp)
qt4_automoc(${papatexturereader_SRCS})
add_executable(papatexturereader ${papatexturereader_SRCS})
target_link_libraries(papatexturereader ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY})
install(TARGETS papatexturereader RUNTIME DESTINATION bin)
