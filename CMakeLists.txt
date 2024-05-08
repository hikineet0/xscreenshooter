cmake_minimum_required(VERSION 3.15)
project(XScreenshooter)

include_directories(lib)
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/lib/*.c")

# GTK3
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})
 set(LIBRARIES ${LIBRARIES} ${GTK3_LIBRARIES})

# X11
find_package(X11 REQUIRED)
include_directories(${X11_INCLUDE_DIR})
link_libraries(${X11_LIBRARIES})
link_libraries(${X11_Xfixes_LIB})

# Curl
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})
link_libraries(${CURL_LIBRARIES})

add_executable(xscreenshooter "${CMAKE_SOURCE_DIR}/src/main.c" ${SOURCES})
target_link_libraries(xscreenshooter ${LIBRARIES})

set(HOME "$ENV{HOME}")
make_directory("${HOME}/.config/xscreenshooter")
file(RENAME "${CMAKE_SOURCE_DIR}/hosts" "${HOME}/.config/xscreenshooter/hosts")


message(STATUS "GTK include directories: ${GTK3_INCLUDE_DIRS}")
message(STATUS "X11 include directory = ${X11_INCLUDE_DIR}")
message(STATUS "Xfixes include directory = ${X11_Xfixes_LIB}")
message(STATUS "CURL include directory = ${CURL_INCLUDE_DIRS}")
